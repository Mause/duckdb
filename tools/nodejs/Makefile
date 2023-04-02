all: build

src/duckdb/src/include/duckdb.hpp:
	./configure

./node_modules:
	npm install --ignore-scripts
	./node_modules/.bin/node-pre-gyp configure

build: src/duckdb/src/include/duckdb.hpp ./node_modules
	./node_modules/.bin/node-pre-gyp build -j max --loglevel=silent

debug: src/duckdb/src/include/duckdb.hpp ./node_modules
	./node_modules/.bin/node-pre-gyp build -j max --debug --verbose

clean:
	@rm -rf ./build
	rm -rf lib/binding/
	rm -f test/support/big.db-journal
	rm -rf ./node_modules/
	rm -rf src/duckdb/
	rm -f binding.gyp

complete_build:
	./configure
	npm install

test:
	npm test

check: test

.PHONY: test clean build
