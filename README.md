# Minecraft Client Loader

Open-source loader for Minecraft clients built with Python and PyQt6.

## Features

- **Java Version Selection**: Choose between Java 8, 11, 17, or 21
- **Custom Installation Directory**: Select where to install the client (defaults to C:/MinecraftClient if skipped)
- **Skip Option**: Skip setup steps if needed
- **Bilingual Interface**: Switch between Russian (RU) and English (EN)
- **Dark Theme**: Modern dark interface with black and soft dark green colors
- **Progress Tracking**: Visual progress bar showing download and extraction status
- **Automatic Download & Extraction**: Downloads client archive and extracts it automatically
- **Client Launch**: Launches the JAR file after installation

## Requirements

- Python 3.8+
- PyQt6
- python-dotenv

## Installation

1. Clone or download this repository
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

3. Configure the loader by editing the `.env` file:
   ```env
   # URL to download the client archive (ZIP file)
   CLIENT_DOWNLOAD_URL=https://example.com/client.zip
   
   # Name of the JAR file inside the archive
   CLIENT_JAR_NAME=client.jar
   
   # Default installation directory
   DEFAULT_INSTALL_DIR=C:/MinecraftClient
   
   # Client display name
   CLIENT_DISPLAY_NAME=My Minecraft Client
   ```

## Usage

Run the loader:
```bash
python minecraft_loader.py
```

### First Run

1. **Select Java Version**: Choose from Java 8, 11, 17, or 21
2. **Select Installation Directory**: Click "Browse..." to choose a location, or click "Skip" to use the default
3. **Main Page**: After setup, you'll see the main page with:
   - Client name displayed in bold white text
   - Rounded "Launch" button
   - Progress bar showing download/installation status
   - Language selector in the top-right corner (RU/EN)

### Configuration

All configuration is done through the `.env` file:

| Variable | Description | Example |
|----------|-------------|---------|
| `CLIENT_DOWNLOAD_URL` | Direct link to the client ZIP archive | `https://example.com/client.zip` |
| `CLIENT_JAR_NAME` | Path to the JAR file inside the archive | `client.jar` or `folder/client.jar` |
| `DEFAULT_INSTALL_DIR` | Default installation path if user skips selection | `C:/MinecraftClient` |
| `CLIENT_DISPLAY_NAME` | Name displayed on the main page | `My Minecraft Client` |

## Project Structure

```
minecraft-loader/
├── minecraft_loader.py    # Main application code
├── .env                   # Configuration file (edit this!)
├── requirements.txt       # Python dependencies
└── README.md             # This file
```

## License

MIT License - Free and Open Source

## Notes

- The loader downloads the archive from the specified URL and extracts it to the selected directory
- If no directory is selected, files are installed to the default directory (C:/MinecraftClient on Windows)
- The JAR file path inside the archive should be specified relative to the archive root
- Java version selection currently maps to the system's `java` command (can be extended to use specific Java installations)
