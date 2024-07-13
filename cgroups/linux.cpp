#include "linux.hpp"

#include "Path.h"
#include "duckdb/common/file_system.hpp"
#include "duckdb/common/operator/aggregate_operators.hpp"
#include "duckdb/common/operator/numeric_binary_operators.hpp"
#include "duckdb/common/string_util.hpp"

#include <cmath>
#include <csignal>
#include <thread>

using namespace duckdb;

usize logical_cpus() {
	cpu_set_t set;

	if (sched_getaffinity(0, sizeof(cpu_set_t), &set) == 0) {
		uint32_t count = 0;
		for (int i = 0; i < CPU_SETSIZE; i++) {
			if (CPU_ISSET(i, &set)) {
				count += 1;
			}
		}
		return count;
	} else {
		auto cpus = sysconf(_SC_NPROCESSORS_ONLN);
		if (cpus < 1) {
			return 1;
		} else {
			return cpus;
		}
	}
}

usize get_num_cpus() {
	auto m = cgroups_num_cpus();
	if (m) {
		return m;
	} else {
		return logical_cpus();
	}
}

usize get_num_physical_cpus(duckdb::FileSystem &fs) {
	const char *string = "/proc/cpuinfo";

	if (!fs.FileExists(string)) {
		return get_num_cpus();
	}

	auto file =
	    fs.OpenFile(string, 0, duckdb::FileLockType::NO_LOCK, duckdb::FileCompressionType::UNCOMPRESSED, nullptr);
	char buffer[1000];
	file->Read(buffer, 1000);
	std::map<long, long> map;
	uint32_t physid = 0;
	usize cores = 0;
	auto chgcount = 0;
	char *line;
	while (std::getline(buffer, line)) {
		auto it = duckdb::StringUtil::Split(line, ':');
		auto key = it[0];
		auto value = it[1];

		if (key == "physical id") {
			chgcount += sscanf(value.c_str(), "%ld", &physid);
		}
		if (key == "cpu cores") {
			chgcount += sscanf(value.c_str(), "%ld", &cores);
		}
		if (chgcount == 2) {
			map.emplace(physid, cores);
			chgcount = 0;
		}
	}

	auto count = 0;
	for (const auto &pair : map) {
		count += pair.second;
	}

	if (count == 0) {
		return get_num_cpus();
	} else {
		return count;
	}
}

/// Cached CPUs calculated from cgroups.
///
/// If 0, check logical cpus.

duckdb::optional_ptr<usize> cgroups_num_cpus() {
	auto cpus = init_cgroups();

	if (cpus > 0) {
		return &cpus;
	} else {
		return nullptr;
	}
}

usize init_cgroups() {
	auto quota = *load_cgroups("/proc/self/cgroup", "/proc/self/mountinfo").get();

	if (quota) {
		if (quota == 0) {
			return 0;
		}

		auto logical = logical_cpus();
		auto count = Min::Operation(quota, logical);

		return count;
	}
}

duckdb::optional_ptr<usize> load_cgroups(char *cgroup_proc, char *mountinfo_proc) {
	auto subsys = Subsys::load_cpu(cgroup_proc);
	if (!subsys) {
		return nullptr;
	}
	auto mntinfo = MountInfo::load_cpu(mountinfo_proc, subsys->version);
	if (!mntinfo) {
		return nullptr;
	}
	auto cgroup = Cgroup::translate(*mntinfo, *subsys);
	if (!cgroup) {
		return nullptr;
	}
	return cgroup->cpu_quota();
}

duckdb::optional_ptr<Cgroup> Cgroup::translate(const MountInfo &mntinfo, const Subsys &subsys) {
	// Translate the subsystem directory via the host paths.
	auto rel_from_root = Path(subsys.base).strip_prefix(mntinfo.root);

	// join(mp.MountPoint, relPath)
	auto path = Path::from(mntinfo.mount_point);
	path.push(rel_from_root);
	return duckdb::optional_ptr<Cgroup>(new Cgroup(mntinfo.version, path));
}
Option<std::string> Cgroup::raw_param(const string &param) {
	auto file = File::open(base.join(param)).ok();

	std::string buf;
	file.read_to_string(&buf).ok();

	return &buf;
}
Option<usize> Cgroup::param(const string &param) {
	auto buf = raw_param(param);
	if (!buf)
		return nullptr;

	duckdb::StringUtil::Trim(*buf);

	return buf->parse().ok();
}
std::pair<usize, usize> Cgroup::max() {
	auto max = raw_param("cpu.max");
	max = max.lines().next().split(' ');

	auto quota = max.next().and_then([&](const string &quota) { return quota.parse().ok(); });
	auto period = max.next().and_then(| period | period.parse().ok());

	return std::make_pair(quota, period);
}
duckdb::optional_ptr<usize> Cgroup::cpu_quota() {
	usize quota_us_, period_us_;
	if (version == CgroupVersion::V1) {
		quota_us_ = *quota_us().get();
		period_us_ = period_us();
	} else {
		quota_us_ = max().first;
		period_us_ = max().second;
	};

	// protect against dividing by zero
	if (period_us_ == 0) {
		return nullptr;
	}

	// Ceil the division, since we want to be able to saturate
	// the available CPUs, and flooring would leave a CPU un-utilized.

	double d = std::ceil(quota_us_ / period_us_);
	return duckdb::optional_ptr<usize>(&d);
}

Option<MountInfo> MountInfo::load_cpu(const Path &proc_path, CgroupVersion version) {
	auto file = File::open(proc_path).ok();
	auto file = BufReader::new (file);

	file.lines()
	    .filter_map(| result | result.ok())
	    .filter_map(MountInfo::parse_line)
	    .find(| mount_info | mount_info.version == version)
}

Option<MountInfo> MountInfo::parse_line(const string &line) {
	auto fields = StringUtil::Split(line, ' ');

	// 7 5 0:6 </> /sys/fs/cgroup/cpu,cpuacct rw,nosuid,nodev,noexec,relatime shared:7 - cgroup cgroup
	// rw,cpu,cpuacct
	auto mnt_root = fields[3];
	// 7 5 0:6 / </sys/fs/cgroup/cpu,cpuacct> rw,nosuid,nodev,noexec,relatime shared:7 - cgroup cgroup
	// rw,cpu,cpuacct
	auto mnt_point = fields.next();

	// Ignore all fields until the separator(-).
	// Note: there could be zero or more optional fields before hyphen.
	// See: https://man7.org/linux/man-pages/man5/proc.5.html
	// 7 5 0:6 / /sys/fs/cgroup/cpu,cpuacct rw,nosuid,nodev,noexec,relatime shared:7 <-> cgroup cgroup
	// rw,cpu,cpuacct Note: we cannot use `?` here because we need to support Rust 1.13.
	match fields.find(| &s | s == "-") {
	    Some(_) = > {} None = > return None,
	};

	// 7 5 0:6 / /sys/fs/cgroup/cpu,cpuacct rw,nosuid,nodev,noexec,relatime shared:7 - <cgroup> cgroup
	// rw,cpu,cpuacct
	auto version = match fields.next() {
	    Some("cgroup") = > CgroupVersion::V1,
	    Some("cgroup2") = > CgroupVersion::V2,
	    _ = > return None,
	};

	// cgroups2 only has a single mount point
	if (version == CgroupVersion::V1) {
		// 7 5 0:6 / /sys/fs/cgroup/cpu,cpuacct rw,nosuid,nodev,noexec,relatime shared:7 - cgroup cgroup
		// <rw,cpu,cpuacct>
		auto super_opts = fields[1];

		// We only care about the 'cpu' option
		if (!Any(StringUtil::Split(super_opts, ','), [&](const string &opt) { return opt == "cpu"; })) {
			return nullptr;
		}
	}

	return MountInfo {version, mnt_root, mnt_point};
}

Option<Subsys> Subsys::load_cpu(const Path &proc_path) {
	BufReader file(File::open(proc_path).ok());

	file.lines()
	    .filter_map(| result | result.ok())
	    .filter_map(Subsys::parse_line)
	    .fold(
	        None, | previous, line | {
		        // already-found v1 trumps v2 since it explicitly specifies its controllers
		        if (previous.is_some() && line.version == CgroupVersion::V2) {
			        return previous;
		        }

		        Some(line)
	        })
}
Option<Subsys> Subsys::parse_line(const string &line) {
	// Example format:
	// 11:cpu,cpuacct:/
	auto fields = StringUtil::Split(line, ':');

	auto sub_systems = fields[1];

	CgroupVersion version;
	if (sub_systems.empty()) {
		version = CgroupVersion::V2;
	} else {
		version = CgroupVersion::V1;
	};

	if (version == CgroupVersion::V1 &&
	    Any(!StringUtil::Split(sub_systems, ','), [&](const string &sub) { return sub == "cpu"; })) {
		return nullptr;
	}

	fields.next().map(| path | Subsys {
		version : version,
		base : path.to_owned(),
	})
}
//
// #[cfg(test)]
// mod tests {
//	mod v1 {
//		use super::super:: {Cgroup, CgroupVersion, MountInfo, Subsys};
//		use std::path:: {Path, PathBuf};
//
//		// `static_in_const` feature is not stable in Rust 1.13.
//		static FIXTURES_PROC : &'static str = "fixtures/cgroups/proc/cgroups";
//
//		                       static FIXTURES_CGROUPS
//		    : &'static str = "fixtures/cgroups/cgroups";
//
//		      macro_rules !join {($base
//		                          : expr,
//		                            $($path
//		                              : expr),
//		                            +) = > ({Path::new ($base) $(.join($path)) + })}
//
// #[test]
//		      fn
//		      test_load_mountinfo() {
//			// test only one optional fields
//			auto path = join !(FIXTURES_PROC, "mountinfo");
//
//			auto mnt_info = MountInfo::load_cpu(path, CgroupVersion::V1).unwrap();
//
//			assert_eq !(mnt_info.root, "/");
//			assert_eq !(mnt_info.mount_point, "/sys/fs/cgroup/cpu,cpuacct");
//
//			// test zero optional field
//			auto path = join !(FIXTURES_PROC, "mountinfo_zero_opt");
//
//			auto mnt_info = MountInfo::load_cpu(path, CgroupVersion::V1).unwrap();
//
//			assert_eq !(mnt_info.root, "/");
//			assert_eq !(mnt_info.mount_point, "/sys/fs/cgroup/cpu,cpuacct");
//
//			// test multi optional fields
//			auto path = join !(FIXTURES_PROC, "mountinfo_multi_opt");
//
//			auto mnt_info = MountInfo::load_cpu(path, CgroupVersion::V1).unwrap();
//
//			assert_eq !(mnt_info.root, "/");
//			assert_eq !(mnt_info.mount_point, "/sys/fs/cgroup/cpu,cpuacct");
//		}
//
// #[test]
//		fn test_load_subsys() {
//			auto path = join !(FIXTURES_PROC, "cgroup");
//
//			auto subsys = Subsys::load_cpu(path).unwrap();
//
//			assert_eq !(subsys.base, "/");
//			assert_eq !(subsys.version, CgroupVersion::V1);
//		}
//
// #[test]
//		fn test_cgroup_mount() {
//			auto cases = &[
//				("/", "/sys/fs/cgroup/cpu", "/", Some("/sys/fs/cgroup/cpu")),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd", Some("/sys/fs/cgroup/cpu"), ),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd/", Some("/sys/fs/cgroup/cpu"), ),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd/large", Some("/sys/fs/cgroup/cpu/large"), ),
//				// fails
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/elsewhere", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd-other-dir", None, ),
//			];
//
//			for
//				&(root, mount_point, subsys, expected)in cases.iter() {
//					auto mnt_info = MountInfo {
//						version : CgroupVersion::V1,
//						root : root.into(),
//						mount_point : mount_point.into(),
//					};
//					auto subsys = Subsys {
//						version : CgroupVersion::V1,
//						base : subsys.into(),
//					};
//
//					auto actual = Cgroup::translate(mnt_info, subsys).map(| c | c.base);
//					auto expected = expected.map(PathBuf::from);
//					assert_eq !(actual, expected);
//				}
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota() {
//			auto cgroup = Cgroup::new (CgroupVersion::V1, join !(FIXTURES_CGROUPS, "good"));
//			assert_eq !(cgroup.cpu_quota(), Some(6));
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota_divide_by_zero() {
//			auto cgroup = Cgroup::new (CgroupVersion::V1, join !(FIXTURES_CGROUPS, "zero-period"));
//			assert !(cgroup.quota_us().is_some());
//			assert_eq !(cgroup.period_us(), Some(0));
//			assert_eq !(cgroup.cpu_quota(), None);
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota_ceil() {
//			auto cgroup = Cgroup::new (CgroupVersion::V1, join !(FIXTURES_CGROUPS, "ceil"));
//			assert_eq !(cgroup.cpu_quota(), Some(2));
//		}
//	}
//
//	mod v2 {
//		use super::super:: {Cgroup, CgroupVersion, MountInfo, Subsys};
//		use std::path:: {Path, PathBuf};
//
//		// `static_in_const` feature is not stable in Rust 1.13.
//		static FIXTURES_PROC : &'static str = "fixtures/cgroups2/proc/cgroups";
//
//		                       static FIXTURES_CGROUPS
//		    : &'static str = "fixtures/cgroups2/cgroups";
//
//		      macro_rules !join {($base
//		                          : expr,
//		                            $($path
//		                              : expr),
//		                            +) = > ({Path::new ($base) $(.join($path)) + })}
//
// #[test]
//		      fn
//		      test_load_mountinfo() {
//			// test only one optional fields
//			auto path = join !(FIXTURES_PROC, "mountinfo");
//
//			auto mnt_info = MountInfo::load_cpu(path, CgroupVersion::V2).unwrap();
//
//			assert_eq !(mnt_info.root, "/");
//			assert_eq !(mnt_info.mount_point, "/sys/fs/cgroup");
//		}
//
// #[test]
//		fn test_load_subsys() {
//			auto path = join !(FIXTURES_PROC, "cgroup");
//
//			auto subsys = Subsys::load_cpu(path).unwrap();
//
//			assert_eq !(subsys.base, "/");
//			assert_eq !(subsys.version, CgroupVersion::V2);
//		}
//
// #[test]
//		fn test_load_subsys_multi() {
//			auto path = join !(FIXTURES_PROC, "cgroup_multi");
//
//			auto subsys = Subsys::load_cpu(path).unwrap();
//
//			assert_eq !(subsys.base, "/");
//			assert_eq !(subsys.version, CgroupVersion::V1);
//		}
//
// #[test]
//		fn test_cgroup_mount() {
//			auto cases = &[
//				("/", "/sys/fs/cgroup/cpu", "/", Some("/sys/fs/cgroup/cpu")),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd", Some("/sys/fs/cgroup/cpu"), ),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd/", Some("/sys/fs/cgroup/cpu"), ),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd/large", Some("/sys/fs/cgroup/cpu/large"), ),
//				// fails
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/elsewhere", None),
//				("/docker/01abcd", "/sys/fs/cgroup/cpu", "/docker/01abcd-other-dir", None, ),
//			];
//
//			for
//				&(root, mount_point, subsys, expected)in cases.iter() {
//					auto mnt_info = MountInfo {
//						version : CgroupVersion::V1,
//						root : root.into(),
//						mount_point : mount_point.into(),
//					};
//					auto subsys = Subsys {
//						version : CgroupVersion::V1,
//						base : subsys.into(),
//					};
//
//					auto actual = Cgroup::translate(mnt_info, subsys).map(| c | c.base);
//					auto expected = expected.map(PathBuf::from);
//					assert_eq !(actual, expected);
//				}
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota() {
//			auto cgroup = Cgroup::new (CgroupVersion::V2, join !(FIXTURES_CGROUPS, "good"));
//			assert_eq !(cgroup.cpu_quota(), Some(6));
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota_divide_by_zero() {
//			auto cgroup = Cgroup::new (CgroupVersion::V2, join !(FIXTURES_CGROUPS, "zero-period"));
//			auto period = cgroup.max().map(| max | max .1);
//
//			assert_eq !(period, Some(0));
//			assert_eq !(cgroup.cpu_quota(), None);
//		}
//
// #[test]
//		fn test_cgroup_cpu_quota_ceil() {
//			auto cgroup = Cgroup::new (CgroupVersion::V2, join !(FIXTURES_CGROUPS, "ceil"));
//			assert_eq !(cgroup.cpu_quota(), Some(2));
//		}
//	}
//}