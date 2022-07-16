var binary = require('@mapbox/node-pre-gyp');
var path = require('path');

const PKG = 'package.json';

const paths = [
    path.resolve(path.join(__dirname, '../' + PKG)),
    path.resolve(path.join(__dirname, '../../' + PKG))
];
for (const path of paths) {
    try {
        var binding_path = binary.find(path);
        if (binding_path)
        break;
    } catch (e) { }
}
if(!binding_path) throw new Error('couldnt find binding')

var binding = require(binding_path);

export default binding;
