import duckdb from "./duckdb-binding";

// some wrappers for compatibilities sake
const _Database = duckdb.Database;
const _Connection = duckdb.Connection;
const _Statement = duckdb.Statement;

export const ERROR = duckdb.ERROR as number;
export const OPEN_READONLY = duckdb.OPEN_READONLY as number;
export const OPEN_READWRITE = duckdb.OPEN_READWRITE as number;
export const OPEN_CREATE = duckdb.OPEN_CREATE as number;
export const OPEN_FULLMUTEX = duckdb.OPEN_FULLMUTEX as number;
export const OPEN_SHAREDCACHE = duckdb.OPEN_SHAREDCACHE as number;
export const OPEN_PRIVATECACHE = duckdb.OPEN_PRIVATECACHE as number;

// Build an argument resolver
function buildResolver(arg: ArgType<unknown>) {
  let validity = arg.validity || null;
  switch (arg.physicalType) {
    case "STRUCT": {
      const tmp: Record<string, any> = {};
      const children: Resolver[] = [];
      for (const attr of arg.children) {
        const child = buildResolver(attr);
        children.push((row) => {
          tmp[attr.name] = child(row);
        });
      }
      if (validity != null) {
        return (row: number) => {
          if (!validity[row]) {
            return null;
          }
          for (const resolver of children) {
            resolver(row);
          }
          return tmp;
        };
      } else {
        return (row: any) => {
          for (const resolver of children) {
            resolver(row);
          }
          return tmp;
        };
      }
    }
    default: {
      if (arg.data === undefined) {
        throw new Error(
          "malformed data view, expected data buffer for argument of type: " +
            arg.physicalType
        );
      }
      const data = arg.data;
      if (validity != null) {
        return (row: number) => (!validity[row] ? null : data[row]);
      } else {
        return (row: number) => data[row];
      }
    }
  }
};

function registration(fun: Function, desc: Description<unknown>) {
  try {
    // Translate argument data
    const argResolvers = [];
    for (const arg of desc.args) {
      argResolvers.push(buildResolver(arg));
    }
    const args = [];
    for (const _ of desc.args) {
      args.push(null);
    }

    // Return type
    desc.ret.validity = new Uint8Array(desc.rows);
    switch (desc.ret.physicalType) {
      case "INT8":
        desc.ret.data = new Int8Array(desc.rows);
        break;
      case "INT16":
        desc.ret.data = new Int16Array(desc.rows);
        break;
      case "INT32":
        desc.ret.data = new Int32Array(desc.rows);
        break;
      case "DOUBLE":
        desc.ret.data = new Float64Array(desc.rows);
        break;
      case "DATE64":
      case "TIME64":
      case "TIMESTAMP":
      case "INT64":
        desc.ret.data = new BigInt64Array(desc.rows);
        break;
      case "UINT64":
        desc.ret.data = new BigUint64Array(desc.rows);
        break;
      case "BLOB":
      case "VARCHAR":
        desc.ret.data = new Array(desc.rows);
        break;
    }

    // Call the function
    for (let i = 0; i < desc.rows; ++i) {
      for (let j = 0; j < desc.args.length; ++j) {
        args[j] = argResolvers[j](i);
      }
      const res = fun(...args);
      desc.ret.data[i] = res;
      desc.ret.validity[i] = res === undefined || res === null ? 0 : 1;
    }
  } catch (error) {
    // work around recently fixed napi bug https://github.com/nodejs/node-addon-api/issues/912
    console.log(desc.ret);
    let msg: any = error;
    if (typeof msg == "object" && msg.message) {
      msg = msg.message;
    }
    throw { name: "DuckDB-UDF-Exception", message: msg };
  }
}

interface ArgType<T> {
  name: string;
  data: Int8Array | Int16Array | Int32Array | Float32Array | Float64Array | BigInt64Array | BigUint64Array | Array<string>;
  validity: Uint8Array;
  physicalType: string;
  children: ArgType<any>[];
}
type Resolver = (row: any) => void;
interface Description<Return> {
  args: ArgType<unknown>[];
  ret: ArgType<Return>;
  rows: number;
}

export class Connection extends _Connection {
  constructor() {
    super();
  }

  run(
    sql: string,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(sql: string, ...params: any[]): this;
  run(
    sql: string,
    params: any,
    callback?: (this: RunResult, err: Error | null) => void
  ) {
    var statement = new Statement(this, sql);
    return statement.run(...arguments);
  }

  all(
    sql: string,
    callback?: (this: Statement, err: Error | null, rows: any[]) => void
  ): this;
  all(
    sql: string,
    params: any,
    callback?: (this: Statement, err: Error | null, rows: any[]) => void
  ): this;
  all(sql: string, ...params: any[]) {
    var statement = new Statement(this, sql);
    return statement.all(...arguments);
  }

  each(
    sql: string,
    callback?: (this: Statement, err: Error | null, row: any) => void,
    complete?: (err: Error | null, count: number) => void
  ): this;
  each(
    sql: string,
    params: any,
    callback?: (this: Statement, err: Error | null, row: any) => void,
    complete?: (err: Error | null, count: number) => void
  ): this;
  each(sql: string, ...params: any[]) {
    var statement = new Statement(this, sql);
    return statement.each(...arguments);
  }

  // this follows the wasm udfs somewhat but is simpler because we can pass data much more cleanly
  register(name: string, return_type: string, fun: Function) {
    // TODO what if this throws an error somewhere? do we need a try/catch?
    return this.register_bulk(name, return_type, registration);
  }
}

function default_connection(o: Database): Connection {
  if (o.default_connection == undefined) {
    o.default_connection = new duckdb.Connection(o);
  }
  return o.default_connection!;
}

interface RunResult {}

export class Database extends _Database {
  default_connection?: Connection;

  constructor(filename: string) {
    super(filename);
  }

  prepare(...args: any[]) {
    return default_connection(this).prepare(...args);
  }

  run(
    sql: string,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(sql: string, ...params: any[]): this;
  run(
    sql: string,
    params: any,
    callback?: (this: RunResult, err: Error | null) => void
  ) {
    default_connection(this).run(sql, params, callback);
    return this;
  }

  each(sql: string, callback?: (this: Statement, err: Error | null, row: any) => void, complete?: (err: Error | null, count: number) => void): this;
  each(sql: string, params: any, callback?: (this: Statement, err: Error | null, row: any) => void, complete?: (err: Error | null, count: number) => void): this;
  each(sql: string, ...params: any[]) {
    default_connection(this).each(sql, ...arguments);
    return this;
  }

  all(
    sql: string,
    callback?: (this: Statement, err: Error | null, rows: any[]) => void
  ): this;
  all(sql: string, ...params: any[]): this;
  all(
    sql: string,
    params: any,
    callback?: (this: Statement, err: Error | null, rows: any[]) => void
  ) {
    default_connection(this).all(sql, params, callback);
    return this;
  }

  exec(sql: string, callback?: (this: Statement, err: Error | null) => void) {
    default_connection(this).exec(sql, callback);
    return this;
  }

  register(name: string, return_type: string, fun: Function) {
    default_connection(this).register(name, return_type, fun);
    return this;
  }

  unregister(...args: any[]) {
    default_connection(this).unregister(...args);
    return this;
  }

  get() {
    throw "get() is not implemented because it's evil";
  }
}

class Statement extends _Statement {
  constructor(connection: Connection, text: string) {
    super(connection, text);
  }

  get() {
    throw "get() is not implemented because it's evil";
  }
}
