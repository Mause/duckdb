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
function buildResolver(arg: ArgType) {
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
}

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

interface ArgType {
  name: string;
  data:
    | Int8Array
    | Int16Array
    | Int32Array
    | Float32Array
    | Float64Array
    | BigInt64Array
    | BigUint64Array
    | Array<string>;
  validity: Uint8Array;
  physicalType: string;
  children: ArgType[];
}
type Resolver = (row: any) => void;
interface Description<Return> {
  args: ArgType[];
  ret: ArgType;
  rows: number;
}

export class Connection extends _Connection {
  run(
    sql: string,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(sql: string, ...params: any[]): this;
  run(
    sql: string,
    params: any,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(...params: any[]) {
    var statement = new Statement(this, params[0] as string);
    statement.run(arguments[0], arguments[1], arguments[2]);
    return this;
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
  all(sql: string, ...params: any[]): this;
  all(...args: any[]): this {
    var statement = new Statement(this, args[0] as string);
    statement.all(...args);
    return this;
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
  each(sql: string, ...params: any[]): this;
  each(...args: any[]) {
    var statement = new Statement(this, args[0] as string);
    statement.each(...args);
    return this;
  }

  // this follows the wasm udfs somewhat but is simpler because we can pass data much more cleanly
  register(name: string, return_type: string, fun: Function) {
    // TODO what if this throws an error somewhere? do we need a try/catch?
    return this.register_bulk(name, return_type, registration);
  }
}

export interface RunResult extends Statement {
  lastID: number;
  changes: number;
}

export class Database extends _Database {
  _default_connection?: Connection;

  get default_connection(): Connection {
    if (this._default_connection == undefined) {
      this._default_connection = new Connection(this);
    }
    return this._default_connection!;
  }

  prepare(
    sql: string,
    callback?: (this: Statement, err: Error | null) => void
  ): Statement;
  prepare(
    sql: string,
    params: any,
    callback?: (this: Statement, err: Error | null) => void
  ): Statement;
  prepare(sql: string, ...params: any[]): Statement;
  prepare() {
    // @ts-ignore
    return this.default_connection.prepare(...arguments);
  }

  run(
    sql: string,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(
    sql: string,
    params: any,
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
  run(sql: string, ...params: any[]): this;
  run() {
    // @ts-ignore
    this.default_connection.run(...arguments);
    return this;
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
    this.default_connection.each(sql, ...arguments);
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
    this.default_connection.all(sql, params, callback);
    return this;
  }

  exec(sql: string, callback?: (err: Error | null) => void) {
    this.default_connection.exec(sql, callback);
    return this;
  }

  register(name: string, return_type: string, fun: Function) {
    this.default_connection.register(name, return_type, fun);
    return this;
  }

  unregister(name: string, cb?: () => void) {
    this.default_connection.unregister(name, cb);
    return this;
  }

  get() {
    throw "get() is not implemented because it's evil";
  }
}

class Statement extends _Statement {
  get() {
    throw "get() is not implemented because it's evil";
  }
}
