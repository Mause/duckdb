import json
from typing import List, Optional
import itertools
from os.path import basename, isfile, exists, isabs, join as pjoin
import os
from auditwheel.lddtree import lddtree
from collections import defaultdict
from auditwheel.repair import WHEEL_INFO_RE, strip_symbols, add_platforms, get_replace_platforms
from auditwheel.main_repair import logger, get_policy_by_name, get_priority_by_name, Patchelf, configure_parser
from auditwheel.patcher import ElfPatcher
from auditwheel.wheel_abi import (
    analyze_wheel_abi,
    NonPlatformWheel,
    get_wheel_elfdata,
    get_external_libs,
    get_versioned_symbols,
    update,
    load_policies,
    elf_file_filter,
    log,
    elf_find_versioned_symbols,
    elf_is_python_extension,
    lddtree_external_references,
    get_symbol_policies,
    versioned_symbols_policy,
    POLICY_PRIORITY_LOWEST,
    POLICY_PRIORITY_HIGHEST,
    get_policy_name,
    WheelAbIInfo,
)

import logging
from rich import print

logging.basicConfig(level=logging.DEBUG)


def get_wheel_elfdata(wheel_fn: str):
    full_elftree = {}
    nonpy_elftree = {}
    full_external_refs = {}
    versioned_symbols = defaultdict(set)  # type: Dict[str, Set[str]]
    uses_ucs2_symbols = False
    uses_PyFPE_jbuf = False

    shared_libraries_in_purelib = []

    platform_wheel = False
    for fn, elf in elf_file_filter([wheel_fn]):
        platform_wheel = True

        # Check for invalid binary wheel format: no shared library should
        # be found in purelib
        so_path_split = fn.split(os.sep)

        # If this is in purelib, add it to the list of shared libraries in
        # purelib
        if "purelib" in so_path_split:
            shared_libraries_in_purelib.append(so_path_split[-1])

        # If at least one shared library exists in purelib, this is going
        # to fail and there's no need to do further checks
        if not shared_libraries_in_purelib:
            log.debug("processing: %s", fn)
            elftree = lddtree(fn)

            for key, value in elf_find_versioned_symbols(elf):
                log.debug("key %s, value %s", key, value)
                versioned_symbols[key].add(value)

            is_py_ext, py_ver = elf_is_python_extension(fn, elf)

            # If the ELF is a Python extention, we definitely need to
            # include its external dependencies.
            if is_py_ext:
                full_elftree[fn] = elftree
                uses_PyFPE_jbuf |= elf_references_PyFPE_jbuf(elf)
                if py_ver == 2:
                    uses_ucs2_symbols |= any(True for _ in elf_find_ucs2_symbols(elf))
                full_external_refs[fn] = lddtree_external_references(elftree, wheel_fn)
            else:
                # If the ELF is not a Python extension, it might be
                # included in the wheel already because auditwheel repair
                # vendored it, so we will check whether we should include
                # its internal references later.
                nonpy_elftree[fn] = elftree

    if not platform_wheel:
        raise NonPlatformWheel

    # If at least one shared library exists in purelib, raise an error
    if shared_libraries_in_purelib:
        raise RuntimeError(
            (
                "Invalid binary wheel, found the following shared "
                "library/libraries in purelib folder:\n"
                "\t%s\n"
                "The wheel has to be platlib compliant in order to be "
                "repaired by auditwheel."
            )
            % "\n\t".join(shared_libraries_in_purelib)
        )

    # Get a list of all external libraries needed by ELFs in the wheel.
    needed_libs = {
        lib
        for elf in itertools.chain(full_elftree.values(), nonpy_elftree.values())
        for lib in elf["needed"]
    }

    for fn in nonpy_elftree:
        # If a non-pyextension ELF file is not needed by something else
        # inside the wheel, then it was not checked by the logic above and
        # we should walk its elftree.
        if basename(fn) not in needed_libs:
            full_elftree[fn] = nonpy_elftree[fn]

        # Even if a non-pyextension ELF file is not needed, we
        # should include it as an external reference, because
        # it might require additional external libraries.
        full_external_refs[fn] = lddtree_external_references(
            nonpy_elftree[fn], wheel_fn
        )

    log.debug("full_elftree:\n%s", json.dumps(full_elftree, indent=4))
    log.debug(
        "full_external_refs (will be repaired):\n%s",
        json.dumps(full_external_refs, indent=4),
    )

    return (
        full_elftree,
        full_external_refs,
        versioned_symbols,
        uses_ucs2_symbols,
        uses_PyFPE_jbuf,
    )



def repair_wheel(
    wheel_path: str,
    abis: List[str],
    lib_sdir: str,
    out_dir: str,
    update_tags: bool,
    patcher: ElfPatcher,
    exclude: List[str],
    strip: bool = False,
) -> Optional[str]:

    external_refs_by_fn = get_wheel_elfdata(wheel_path)[1]

    # Do not repair a pure wheel, i.e. has no external refs
    if not external_refs_by_fn:
        return None

    soname_map = {}  # type: Dict[str, Tuple[str, str]]
    if not isabs(out_dir):
        out_dir = abspath(out_dir)

    wheel_fname = basename(wheel_path)

    if True:
        out_wheel = pjoin(out_dir, wheel_fname)

        dest_dir = wheel_fname + lib_sdir

        if not exists(dest_dir):
            os.mkdir(dest_dir)

        # here, fn is a path to a python extension library in
        # the wheel, and v['libs'] contains its required libs
        for fn, v in external_refs_by_fn.items():
            ext_libs = v[abis[0]]["libs"]  # type: Dict[str, str]
            replacements = []  # type: List[Tuple[str, str]]
            for soname, src_path in ext_libs.items():

                if soname in exclude:
                    logger.info(f"Excluding {soname}")
                    continue

                if src_path is None:
                    raise ValueError(
                        (
                            "Cannot repair wheel, because required "
                            'library "%s" could not be located'
                        )
                        % soname
                    )

                new_soname, new_path = copylib(src_path, dest_dir, patcher)
                soname_map[soname] = (new_soname, new_path)
                replacements.append((soname, new_soname))
            if replacements:
                patcher.replace_needed(fn, *replacements)

            if len(ext_libs) > 0:
                new_rpath = os.path.relpath(dest_dir, os.path.dirname(fn))
                new_rpath = os.path.join("$ORIGIN", new_rpath)
                append_rpath_within_wheel(fn, new_rpath, ctx.name, patcher)

        # we grafted in a bunch of libraries and modified their sonames, but
        # they may have internal dependencies (DT_NEEDED) on one another, so
        # we need to update those records so each now knows about the new
        # name of the other.
        for old_soname, (new_soname, path) in soname_map.items():
            needed = elf_read_dt_needed(path)
            replacements = []
            for n in needed:
                if n in soname_map:
                    replacements.append((n, soname_map[n][0]))
            if replacements:
                patcher.replace_needed(path, *replacements)

        if update_tags:
            breakpoint()
            out_wheel = add_platforms(type('', (), {'path': wheel_fname})(), abis, get_replace_platforms(abis[0]))

        if strip:
            libs_to_strip = [path for (_, path) in soname_map.values()]
            extensions = external_refs_by_fn.keys()
            strip_symbols(itertools.chain(libs_to_strip, extensions))

    return out_wheel



def analyze_wheel_abi(wheel_fn: str) -> WheelAbIInfo:
    (
        elftree_by_fn,
        external_refs_by_fn,
        versioned_symbols,
        has_ucs2,
        uses_PyFPE_jbuf,
    ) = get_wheel_elfdata(wheel_fn)

    external_refs = {
        p["name"]: {"libs": {}, "blacklist": {}, "priority": p["priority"]}
        for p in load_policies()
    }

    for fn in elftree_by_fn.keys():
        update(external_refs, external_refs_by_fn[fn])

    external_libs = get_external_libs(external_refs)
    external_versioned_symbols = get_versioned_symbols(external_libs)
    symbol_policies = get_symbol_policies(
        versioned_symbols, external_versioned_symbols, external_refs
    )
    symbol_policy = versioned_symbols_policy(versioned_symbols)

    # let's keep the highest priority policy and
    # corresponding versioned_symbols
    symbol_policy, versioned_symbols = max(
        symbol_policies, key=lambda x: x[0], default=(symbol_policy, versioned_symbols)
    )

    ref_policy = max(
        (e["priority"] for e in external_refs.values() if len(e["libs"]) == 0),
        default=POLICY_PRIORITY_LOWEST,
    )

    blacklist_policy = max(
        (e["priority"] for e in external_refs.values() if len(e["blacklist"]) == 0),
        default=POLICY_PRIORITY_LOWEST,
    )

    ucs_policy = POLICY_PRIORITY_LOWEST if has_ucs2 else POLICY_PRIORITY_HIGHEST
    pyfpe_policy = (
        POLICY_PRIORITY_LOWEST if uses_PyFPE_jbuf else POLICY_PRIORITY_HIGHEST
    )

    ref_tag = get_policy_name(ref_policy)
    sym_tag = get_policy_name(symbol_policy)
    ucs_tag = get_policy_name(ucs_policy)
    pyfpe_tag = get_policy_name(pyfpe_policy)
    blacklist_tag = get_policy_name(blacklist_policy)
    overall_tag = get_policy_name(
        min(symbol_policy, ref_policy, ucs_policy, pyfpe_policy, blacklist_policy)
    )

    return WheelAbIInfo(
        overall_tag,
        external_refs,
        ref_tag,
        versioned_symbols,
        sym_tag,
        ucs_tag,
        pyfpe_tag,
        blacklist_tag,
    )


def execute(args, p):
    for wheel_file in args.WHEEL_FILE:
        if not isfile(wheel_file):
            p.error("cannot access %s. No such file" % wheel_file)

        logger.info("Repairing %s", basename(wheel_file))

        if not exists(args.WHEEL_DIR):
            os.makedirs(args.WHEEL_DIR)

        try:
            wheel_abi = analyze_wheel_abi(wheel_file)
        except NonPlatformWheel:
            logger.info(NonPlatformWheel.LOG_MESSAGE)
            return 1

        policy = get_policy_by_name(args.PLAT)
        reqd_tag = policy["priority"]

        if reqd_tag > get_priority_by_name(wheel_abi.sym_tag):
            msg = (
                'cannot repair "%s" to "%s" ABI because of the presence '
                "of too-recent versioned symbols. You'll need to compile "
                "the wheel on an older toolchain." % (wheel_file, args.PLAT)
            )
            p.error(msg)

        if reqd_tag > get_priority_by_name(wheel_abi.ucs_tag):
            msg = (
                'cannot repair "%s" to "%s" ABI because it was compiled '
                "against a UCS2 build of Python. You'll need to compile "
                "the wheel against a wide-unicode build of Python."
                % (wheel_file, args.PLAT)
            )
            p.error(msg)

        if reqd_tag > get_priority_by_name(wheel_abi.blacklist_tag):
            msg = (
                'cannot repair "%s" to "%s" ABI because it depends on '
                "black-listed symbols." % (wheel_file, args.PLAT)
            )
            p.error(msg)

        abis = [policy["name"]] + policy["aliases"]
        if not args.ONLY_PLAT:
            if reqd_tag < get_priority_by_name(wheel_abi.overall_tag):
                logger.info(
                    (
                        "Wheel is eligible for a higher priority tag. "
                        "You requested %s but I have found this wheel is "
                        "eligible for %s."
                    ),
                    args.PLAT,
                    wheel_abi.overall_tag,
                )
                higher_policy = get_policy_by_name(wheel_abi.overall_tag)
                abis = [higher_policy["name"]] + higher_policy["aliases"] + abis

        patcher = Patchelf()
        out_wheel = repair_wheel(
            wheel_file,
            abis=abis,
            lib_sdir=args.LIB_SDIR,
            out_dir=args.WHEEL_DIR,
            update_tags=args.UPDATE_TAGS,
            patcher=patcher,
            exclude=args.EXCLUDE,
            strip=args.STRIP,
        )

        if out_wheel is not None:
            logger.info("\nFixed-up wheel written to %s", out_wheel)


def main():
    from argparse import ArgumentParser
    p = ArgumentParser()
    # p.add_argument('WHEEL_FILE', action='store_list')
    # p.add_argument('--WHEEL_DIR', default='wheel_dir/')
    configure_parser(p.add_subparsers())
    print(execute(p.parse_args(), log))


if __name__ == "__main__":
    main()
