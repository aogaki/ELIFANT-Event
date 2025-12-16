# chSettings.json Editor

A simple React + Material-UI web application for visually editing two-dimensional array JSON files (such as chSettings.json) used in ELIFANT2025/ELIFANT-Event.

## Features
- Load any chSettings.json file (2D array: [module][channel])
- Select a module and view/edit all its channels in a spreadsheet-like table
- **Apply common values to all channels** - Set a property value once and apply it to all channels in a module
- Edit values directly in the table (supports numbers, strings, and JSON objects)
- Sticky key column for easy navigation
- Material Design UI for a modern look
- Download/save the modified JSON file

## Getting Started

### 1. Install dependencies
```sh
cd json-editor
npm install
```

### 2. Start the development server
```sh
npm start
```

The app will open in your browser (usually at http://localhost:3000).

### 3. Usage
- Click the file input to load your `chSettings.json` file.
- When the file dialog opens, navigate to your project root directory (e.g., ELIFANT-Event) and select the desired `chSettings.json` file from there (such as `all_run_p91Zr/chSettings.json`, `build/chSettings.json`, `E9/chSettings.json`, or `JSON/chSettings.json`).
- Use the module selector to choose a module.
- **Apply common values (NEW):**
  - In the blue "Apply Common Value" section, select a property from the dropdown
  - Enter the desired value (the input type adapts to the property type)
  - Click "Apply to All Channels" to set that value for all channels in the module
- Edit individual channel values directly in the table below.
- Click "Save JSON" to download the updated file.

> **Note:** For security reasons, browsers do not allow web applications to set the default open directory for file dialogs. Please manually navigate to your project root directory when opening files.

## Requirements
- Node.js (v16 or later recommended)
- npm

## Project Structure
- `src/App.js` — Main React component and UI logic
- `public/index.html` — HTML template
- `package.json` — Project dependencies and scripts

## License
MIT
