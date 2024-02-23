function get_class_byte(classname) {
    classname = classname.trim().toLowerCase();
    if (classname == "block") {
        return 0xFF;
    } else if (classname == "round") {
        return 0xFE;
    } else {
        throw "Unknown class: " + classname;
    }

}

function concat_arrays(a, b) { // a, b TypedArray of same type
    var c = new (a.constructor)(a.length + b.length);
    c.set(a, 0);
    c.set(b, a.length);
    return c;
}

function append_byte(ui8a, byte) {
    var b = new Uint8Array(1);
    b[0] = byte;
    return concat_arrays(ui8a, b);
}

class MapBuffer {
    constructor(magic, version) {
        this.data = new Uint8Array(0);
        this._header(magic, version);
    }

    _push(byte) {
        var b = new Uint8Array(1);
        b[0] = byte;
        this.data = concat_arrays(this.data, b);
    }

    _header(magic, version) {
        this._push(magic.charCodeAt(0));
        this._push(magic.charCodeAt(1));
        this._push(magic.charCodeAt(2));
        this._push(magic.charCodeAt(3));
        this._push(version);
    }

    get buffer() {
        return this.data.buffer;
    }

    add_object(object) {
        let type = object.className.trim().toLowerCase();
        switch (type) {
            case "block":
                this._add_block(object);
                break;
            case "round":
                this._add_round(object);
                break;
            default:
                throw "Unknown class: " + object.type;
        }
    }

    objects_count(count) {
        this._push(count);
    }

    _object_lives(object) {
        let lives = object.property("lives");
        if (!lives) {
            lives = 1;
        }
        return lives;
    }

    _add_block(object) {
        this._push(0xFF);
        this._push(object.pos.x + object.size.width / 2);
        this._push(object.pos.y + object.size.height / 2);
        this._push(object.size.width);
        this._push(object.size.height);
        this._push(this._object_lives(object));
    }

    _add_round(object) {
        this._push(0xFE);
        let r = object.size.width / 2;
        this._push(object.pos.x + r);
        this._push(object.pos.y + r);
        this._push(r);
        this._push(this._object_lives(object));
    }
}

var customMapFormat = {
    name: "Flipper Air Arkanoid Map",
    extension: "flaam",

    write: function (map, fileName) {
        var m = new MapBuffer("FLAA", 1);

        for (var i = 0; i < map.layerCount; ++i) {
            var layer = map.layerAt(i);
            if (layer.isObjectLayer) {
                m.objects_count(layer.objectCount);
                for (var j = 0; j < layer.objectCount; ++j) {
                    m.add_object(layer.objectAt(j));
                }
            }
        }

        tiled.log(m.data);

        var file = new BinaryFile(fileName, BinaryFile.WriteOnly);
        file.write(m.buffer);
        file.commit();
    },
}

tiled.registerMapFormat("flaam", customMapFormat)