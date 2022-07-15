var assert = require('assert');
var fs = require('fs');
var pathExists = require('fs').existsSync || require('path').existsSync;

function isFileDNE(err: any) {
    return err.code === 'ENOENT' && err.syscall === 'unlink';
}

export function deleteFile(name: string) {
    try {
        fs.unlinkSync(name);
    } catch(err) {
        if (!isFileDNE(err)) {
            throw err;
        }
    }
};

export function ensureExists(name:string, cb: unknown) {
    if (!pathExists(name)) {
        fs.mkdirSync(name);
    };
}

assert.fileDoesNotExist = function(name: string) {
    try {
        fs.statSync(name);
    } catch(err) {
        if (!isFileDNE(err)) {
            throw err;
        }
    }
};

assert.fileExists = function(name: string) {
    try {
        fs.statSync(name);
    } catch(err) {
        throw err;
    }
};