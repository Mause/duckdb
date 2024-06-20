//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/logging/http_logger.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/fstream.hpp"
#include "duckdb/common/mutex.hpp"
#include "duckdb/common/printer.hpp"
#include "duckdb/main/client_context.hpp"

#include <functional>

namespace duckdb {

//! This has to be templated because we have two namespaces:
//! 1. duckdb_httplib
//! 2. duckdb_httplib_openssl
//! These have essentially the same code, but we cannot convert between them
//! We get around that by templating everything, which requires implementing everything in the header
class HTTPLogger {
	std::function<void(const string &, const string &)> logger;

public:
	explicit HTTPLogger(ClientContext &context_p) : context(context_p) {
	}

public:
	template <class REQUEST, class RESPONSE>
	std::function<void(const REQUEST &, const RESPONSE &)> GetLogger() {
		return [&](const REQUEST &req, const RESPONSE &res) {
			Log(req, res);
		};
	}
	void SetLogger(std::function<void(const string &, const string &)> logger) {
		this->logger = std::move(logger);
	}

private:
	template <class STREAM, class REQUEST, class RESPONSE>
	static inline void TemplatedWriteRequests(STREAM &out, const REQUEST &req, const RESPONSE &res) {
		TemplatedWriteRequest(out, req);
		out << "\n";
		TemplatedWriteResponse(out, res);
	}
	template <class STREAM, class REQUEST>
	static inline void TemplatedWriteRequest(STREAM &out, const REQUEST &req) {
		out << "HTTP Request:\n";
		out << "\t" << req.method << " " << req.path << "\n";
		for (auto &entry : req.headers) {
			out << "\t" << entry.first << ": " << entry.second << "\n";
		}
	}
	template <class STREAM, class RESPONSE>
	static inline void TemplatedWriteResponse(STREAM &out, const RESPONSE &res) {
		out << "\nHTTP Response:\n";
		out << "\t" << res.status << " " << res.reason << " " << res.version << "\n";
		for (auto &entry : res.headers) {
			out << "\t" << entry.first << ": " << entry.second << "\n";
		}
		out << "\n";
	}

	template <class REQUEST, class RESPONSE>
	void Log(const REQUEST &req, const RESPONSE &res) {
		const auto &config = ClientConfig::GetConfig(context);
		D_ASSERT(config.enable_http_logging);

		lock_guard<mutex> guard(lock);
		if (config.http_logging_output.empty()) {
			stringstream out;
			TemplatedWriteRequests(out, req, res);
			if (logger) {
				stringstream request;
				TemplatedWriteRequest(request, req);
				stringstream response;
				TemplatedWriteResponse(response, res);
				logger(request.str(), response.str());
			} else {
				Printer::Print(out.str());
			}
		} else {
			ofstream out(config.http_logging_output, ios::app);
			TemplatedWriteRequests(out, req, res);
			out.close();
			// Throw an IO exception if it fails to write to the file
			if (out.fail()) {
				throw IOException("Failed to write HTTP log to file \"%s\": %s", config.http_logging_output,
				                  strerror(errno));
			}
		}
	}

private:
	ClientContext &context;
	mutex lock;
};

} // namespace duckdb
