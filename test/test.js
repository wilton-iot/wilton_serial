
define(["wilton/dyload", "wilton/thread", "wilton/serial/Serial"], function(dyload, thread, Serial) {
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

            var written_recall = ser.write("$RECALL\r\n");
            print(written_recall);
            var written_start = ser.write("$START\r\n");
            print(written_start);

            var resp = "";
            do {
                resp = ser.readLine();
                print(resp);
            } while(resp.length > 0);
            
            thread.sleepMillis(5000);
            
            ser.close();
        }
    };
});
