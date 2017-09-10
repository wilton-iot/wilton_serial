
define(["wilton/dyload", "wilton/serial/Serial"], function(dyload, Serial) {
    return {
        main: function() {
            dyload({
                name: "wilton_serial"
            });

            var ser = new Serial({
                port: "/dev/ttyUSB1",
                baudRate: 4800,
                parity: "NONE", // "EVEN", "ODD", "MARK", "SPACE"
                byteSize: 8,
                stopBitsCount: 1,
                readTimeoutMillis: 1000,
                writeTimeoutMillis: 1000
            });

            ser.write("$RECALL\r\n");
            ser.write("$START\r\n");

            var resp = "";
            do {
                resp = ser.readLine();
            } while(resp.length > 0);
            
            ser.close();
        }
    };
});
