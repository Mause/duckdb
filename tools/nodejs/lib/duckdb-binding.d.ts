declare module "./duckdb-bindings";

export type StatementCallback = (this: Statement, err: Error | null) => void;

export class Database {
  constructor(filename: string, callback?: (err: Error | null) => void);
  constructor(
    filename: string,
    config?: Record<string, any>,
    callback?: (err: Error | null) => void
  );

  close(callback?: (err: Error | null) => void): void;

  wait(): void;
  serialize(callback?: () => void): void;
  parallelize(callback?: () => void): void;
  connect(): void;
  interrupt(): void;
}
export class Connection {
  constructor(db: Database);

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

  exec(
    sql: string,
    callback?: (this: Statement, err: Error | null) => void
  ): this;

  register_bulk(name: string, return_type: string, func: Function): void;
  unregister(name: string, cb?: StatementCallback): void;
}
export class Statement {
  sql: string;

  constructor(connection: Connection, text: string);

  run(
    callback?: (this: RunResult, err: Error | null) => void
  ): this;
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

  all(callback?: (err: Error | null, rows: any[]) => void): this;
  all(
    params: any,
    callback?: (this: RunResult, err: Error | null, rows: any[]) => void
  ): this;
  all(...params: any[]): this;

  each(
    callback?: (err: Error | null, row: any) => void,
    complete?: (err: Error | null, count: number) => void
  ): this;
  each(
    params: any,
    callback?: (this: RunResult, err: Error | null, row: any) => void,
    complete?: (err: Error | null, count: number) => void
  ): this;
  each(...params: any[]): this;

  finalize(callback?: (err: Error) => void): Database;
}

export const ERROR: number;
export const OPEN_READONLY: number;
export const OPEN_CREATE: number;
export const OPEN_PRIVATECACHE: number;
export const OPEN_READWRITE: number;
export const OPEN_FULLMUTEX: number;
export const OPEN_SHAREDCACHE: number;
