#pragma once

#include "Path.h"
#include "duckdb/common/optional_ptr.hpp"

#include <cstddef>
#include <functional>

typedef size_t usize;
class Subsys;
duckdb::optional_ptr<usize> cgroups_num_cpus();
duckdb::optional_ptr<usize> load_cgroups(char *cgroup_proc, char *mountinfo_proc);
usize init_cgroups();

enum CgroupVersion {
	V1,
	V2,
};

typedef duckdb::string PathBuf;
template <typename T>
using Option = duckdb::optional_ptr<T>;

class Subsys {
public:
	CgroupVersion version;
	duckdb::string base;

	static Option<Subsys> load_cpu(const Path &proc_path);

	Option<Subsys> parse_line(const duckdb::string &line);
};

class MountInfo {
public:
	CgroupVersion version;
	duckdb::string root;
	duckdb::string mount_point;

	static Option<MountInfo> load_cpu(const Path &proc_path, CgroupVersion version);

	static Option<MountInfo> parse_line(const std::string &line);
};

class Cgroup {
public:
	CgroupVersion version;
	Path base;

	Cgroup(CgroupVersion version, Path dir) : version(version), base(dir) {
	}

	static duckdb::optional_ptr<Cgroup> translate(const MountInfo &mntinfo, const Subsys &subsys);

	duckdb::optional_ptr<usize> cpu_quota();

	duckdb::optional_ptr<usize> quota_us() {
		return param("cpu.cfs_quota_us");
	}

	duckdb::optional_ptr<usize> period_us() {
		return param("cpu.cfs_period_us");
	}

	std::pair<usize, usize> max();

	Option<usize> param(const std::string &param);

	Option<std::string> raw_param(const std::string &param);
};

template <typename T>
bool Any(const std::vector<T> &source, std::function<bool(T)> callback) {
	for (const auto &child : source) {
		if (callback(child)) {
			return true;
		}
	}
	return false;
}
