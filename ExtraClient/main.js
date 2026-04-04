const net = require('net');
const config = require('../config');

const client = new net.Socket();
const EXPECTED_BYTES = config.MATRIX_SIZE * config.MATRIX_SIZE * 8;
let bytesReceived = 0;
let step = 0;

function createCmd(text) {
    const b = Buffer.alloc(10, ' ');
    b.write(text);
    return b;
}

client.connect(config.SERVER_PORT, config.SERVER_IP, () => {
    console.log("Connected to server!");
    console.log("Step 1: Sending configuration...");
    client.write(createCmd('CONFIG'));
    const b = Buffer.alloc(8);
    b.writeUInt32BE(config.MATRIX_SIZE, 0);
    b.writeUInt32BE(config.THREAD_COUNT, 4);
    client.write(b);
    step = 1;
});

client.on('data', (data) => {
    if (step === 5) {
        bytesReceived += data.length;
        if (bytesReceived >= EXPECTED_BYTES) {
            console.log("Result received successfully.");
            client.destroy();
        }
        return;
    }

    const resp = data.toString().trim();
    if (resp.includes('CONFIG_OK') && step === 1) {
        console.log("Server: CONFIG_OK");
        console.log("Step 2: Sending random data...");
        client.write(createCmd('DATA'));
        const dataBuf = Buffer.alloc(EXPECTED_BYTES);
        for (let i = 0; i < config.MATRIX_SIZE * config.MATRIX_SIZE; i++) {
            dataBuf.writeDoubleBE(Math.random() * 9 + 1, i * 8);
        }
        client.write(dataBuf);
        step = 2;
    } 
    else if (resp.includes('DATA_OK') && step === 2) {
        console.log("Server: DATA_OK");
        console.log("Step 3: Starting computations...");
        client.write(createCmd('START'));
        step = 3;
    }
    else if (resp.includes('STARTED') && step === 3) {
        console.log("Server: STARTED");
        const poller = setInterval(() => client.write(createCmd('STATUS')), 500);
        step = 4;
        
        client.on('data', (d) => {
            const s = d.toString().trim();
            if (step === 4) console.log("Status: " + s);
            if (s.includes('DONE') && step === 4) {
                clearInterval(poller);
                console.log("Step 5: Receiving result matrix...");
                client.write(createCmd('RESULT'));
                step = 5;
            }
        });
    }
});
