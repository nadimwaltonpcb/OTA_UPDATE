const express = require('express');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 3000;

// Path to serve the static folder
const folderToServe = path.join('E:', 'NADIM', 'OTA_UPDATE', 'ota-server');

// Serve the folder as static content
app.use(express.static(folderToServe));

// Endpoint to check server status and show files
app.get('/', (req, res) => {
    res.send('OTA Update Server is running...');
});

// Endpoint to list all files in the directory
app.get('/files', (req, res) => {
    fs.readdir(folderToServe, (err, files) => {
        if (err) {
            return res.status(500).send('Unable to read directory');
        }

        // Generate a simple HTML listing of files
        const fileListHtml = files.map(file => {
            return `<li><a href="/${file}" target="_blank">${file}</a></li>`;
        }).join('');

        res.send(`
            <h1>Files in OTA Server Directory</h1>
            <ul>
                ${fileListHtml}
            </ul>
        `);
    });
});

// Start the server
app.listen(PORT, () => {
    console.log(`Server is running on http://localhost:${PORT}`);
    console.log(`Serving files from: ${folderToServe}`);
});
