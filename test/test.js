
// valgrind --track-fds=yes --leak-check=yes --show-reachable=yes --track-origins=yes --suppressions=../deps/cmake/resources/valgrind/popt_poptGetNextOpt.supp --suppressions=../deps/cmake/resources/valgrind/wilton_dyload.supp --suppressions=../deps/cmake/resources/valgrind/openssl_malloc.supp ./bin/wilton_cli ../modules/wilton_serial/test/test.js -m ../js
define(["wilton/hex", "wilton/Serial"], function(hex, Serial) {
    return {
        main: function() {

            var ser = new Serial({
                port: "/dev/ttyUSB1",
                baudRate: 4800,
                parity: "NONE", // "EVEN", "ODD", "MARK", "SPACE"
                byteSize: 8,
                stopBitsCount: 1,
                timeoutMillis: 1000
            });

            var written_recall = ser.writePlain("$RECALL\r\n");
            print(written_recall);
            var written_start = ser.writePlain("$START\r\n");
            print(written_start);
            
            for (var i = 0; i < 16; i++) {
                print(i + ": [" + hex.decodeBytes(ser.readLine()) + "]");
            }
            
            ser.close();
        }
    };
});
