# https://central.sonatype.org/pages/manual-staging-bundle-creation-and-deployment.html
# https://issues.sonatype.org/browse/OSSRH-58179

# this is the pgp key we use to sign releases
# if this key should be lost, generate a new one with `gpg --full-generate-key`
# AND upload to keyserver: `gpg --keyserver hkp://keys.openpgp.org --send-keys [...]`
# export the keys for GitHub Actions like so: `gpg --export-secret-keys | base64`
# --------------------------------
# pub   ed25519 2022-02-07 [SC]
#       65F91213E069629F406F7CF27F610913E3A6F526
# uid           [ultimate] DuckDB <quack@duckdb.org>
# sub   cv25519 2022-02-07 [E]

import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from os import makedirs
from os.path import join
import re


version_regex = re.compile(r'^v(\d+\.\d+\.\d+)$')
sys.argv.append('main')
sys.argv.append('artifacts')
sys.argv.append('tools/jdbc')


def exec(cmd):
    print(cmd)

    try:
        stdout = subprocess.run(
            cmd.split(' '),
            check=True,
            stdout=subprocess.PIPE,
            text=True,
            stderr=subprocess.PIPE,
            cwd=cwd,
        ).stdout
        print(stdout)
        return stdout

    except subprocess.CalledProcessError as e:
        print(e.output)
        raise Exception(e.output)


if len(sys.argv) < 4 or not os.path.isdir(sys.argv[2]) or not os.path.isdir(sys.argv[3]):
    print("Usage: [release_tag, format: v1.2.3] [artifact_dir] [jdbc_root_path]")
    exit(1)

version_regex = re.compile(r'^v((\d+)\.(\d+)\.\d+)$')
cwd = sys.argv[3]
breakpoint()
staging_dir = join(cwd, 'target')
makedirs(staging_dir, exist_ok=True)
release_tag = sys.argv[1]
deploy_url = 'https://oss.sonatype.org/service/local/staging/deploy/maven2/'
is_release = True

if release_tag == 'main':
    # for SNAPSHOT builds we increment the minor version and set patch level to zero.
    # seemed the most sensible
    last_tag = exec('git tag --sort=-committerdate').split('\n')[0]
    re_result = version_regex.search(last_tag)
    if re_result is None:
        raise ValueError("Could not parse last tag %s" % last_tag)
    release_version = "%d.%d.0-SNAPSHOT" % (int(re_result.group(2)), int(re_result.group(3)) + 1)
    # orssh uses a different deploy url for snapshots yay
    deploy_url = 'https://oss.sonatype.org/content/repositories/snapshots/'
    is_release = False
elif version_regex.match(release_tag):
    release_version = version_regex.search(release_tag).group(1)
else:
    print("Not running on %s" % release_tag)
    exit(0)

jdbc_artifact_dir = sys.argv[2]
jdbc_root_path = sys.argv[3]

combine_builds = ['linux-amd64', 'osx-universal', 'windows-amd64', 'linux-aarch64']

staging_dir = tempfile.mkdtemp()

binary_jar = '%s/duckdb_jdbc-%s.jar' % (staging_dir, release_version)
sources_jar = '%s/duckdb_jdbc-%s-sources.jar' % (staging_dir, release_version)
javadoc_jar = '%s/duckdb_jdbc-%s-javadoc.jar' % (staging_dir, release_version)

# create a matching POM with this version
exec(f"mvn versions:set -DnewVersion={release_version}")

# fatten up jar to add other binaries, start with first one
shutil.copyfile(os.path.join(jdbc_artifact_dir, "java-" + combine_builds[0], "duckdb_jdbc.jar"), binary_jar)
for build in combine_builds[1:]:
    old_jar = zipfile.ZipFile(os.path.join(jdbc_artifact_dir, "java-" + build, "duckdb_jdbc.jar"), 'r')
    for zip_entry in old_jar.namelist():
        if zip_entry.startswith('libduckdb_java.so'):
            old_jar.extract(zip_entry, staging_dir)
            exec("jar -uf %s -C %s %s" % (binary_jar, staging_dir, zip_entry))

javadoc_stage_dir = tempfile.mkdtemp()

exec("javadoc -Xdoclint:-reference -d %s -sourcepath %s/src/main/java org.duckdb" % (javadoc_stage_dir, jdbc_root_path))
exec("jar -cvf %s -C %s ." % (javadoc_jar, javadoc_stage_dir))
exec("jar -cvf %s -C %s/src/main/java org" % (sources_jar, jdbc_root_path))

# make sure all files exist before continuing
for path in (javadoc_jar, sources_jar, binary_jar):
    if not os.path.exists(path):
        raise ValueError(f'could not create all required files: {path}')
breakpoint()

# run basic tests, it should now work on whatever platform this is
exec("java -cp %s org.duckdb.test.TestDuckDBJDBC" % binary_jar)

# now sign and upload everything
# for this to work, you must have entry in ~/.m2/settings.xml:

# <settings xmlns="http://maven.apache.org/SETTINGS/1.0.0"
#   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
#   xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.0.0
#   https://maven.apache.org/xsd/settings-1.0.0.xsd">
#   <servers>
#     <server>
#       <id>ossrh</id>
#       <username>hfmuehleisen</username> <!-- Sonatype OSSRH JIRA user/pw -->
#       <password>[...]</password>
#     </server>
#   </servers>
# </settings>

results_dir = os.path.join(jdbc_artifact_dir, "results")
if not os.path.exists(results_dir):
    os.mkdir(results_dir)


for jar in [binary_jar, sources_jar, javadoc_jar]:
    shutil.copyfile(jar, os.path.join(results_dir, os.path.basename(jar)))

print("JARs created, uploading (this can take a while!)")
deploy_cmd_prefix = 'mvn gpg:sign-and-deploy-file -Durl=%s -DrepositoryId=ossrh' % deploy_url
exec("%s -Dfile=%s" % (deploy_cmd_prefix, binary_jar))
exec("%s -Dclassifier=sources -Dfile=%s" % (deploy_cmd_prefix, sources_jar))
exec("%s -Dclassifier=javadoc -Dfile=%s" % (deploy_cmd_prefix, javadoc_jar))


if not is_release:
    print("Not a release, not closing repo")
    exit(0)

print("Close/Release steps")
# # beautiful
os.environ["MAVEN_OPTS"] = '--add-opens=java.base/java.util=ALL-UNNAMED'

# this list has horrid output, lets try to parse. What we want starts with orgduckdb- and then a number
repo_id = re.search(r'(orgduckdb-\d+)', exec("mvn nexus-staging:rc-list")).groups()[0]
exec("mvn nexus-staging:rc-close -DstagingRepositoryId=%s" % (repo_id))
exec("mvn nexus-staging:rc-release -DstagingRepositoryId=%s" % (repo_id))

print("Done?")
