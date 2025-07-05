import React, { useState } from "react";
import {
  Box,
  Button,
  FormControl,
  InputLabel,
  MenuItem,
  Select,
  Paper,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  TextField,
  Typography,
  Alert
} from "@mui/material";

function App() {
  const [data, setData] = useState(null);
  const [filename, setFilename] = useState("");
  const [error, setError] = useState("");

  // State for selected module
  const [selectedModule, setSelectedModule] = useState(0);

  // Define property types for chSettings.json
  const propertyTypes = {
    ACChannel: 'integer',
    ACModule: 'integer',
    Channel: 'integer',
    DetectorType: 'string',
    Distance: 'float',
    HasAC: 'boolean',
    ID: 'integer',
    IsEventTrigger: 'boolean',
    Module: 'integer',
    Phi: 'float',
    Tags: 'array',
    Theta: 'float',
    ThresholdADC: 'integer',
    p0: 'float',
    p1: 'float',
    p2: 'float',
    p3: 'float',
    x: 'float',
    y: 'float',
    z: 'float'
  };

  // Convert string value to appropriate type
  const convertToType = (value, type) => {
    if (value === null || value === undefined) return value;
    
    switch (type) {
      case 'integer':
        const intVal = parseInt(value, 10);
        return isNaN(intVal) ? 0 : intVal;
      case 'float':
        const floatVal = parseFloat(value);
        return isNaN(floatVal) ? 0.0 : floatVal;
      case 'boolean':
        if (typeof value === 'boolean') return value;
        return value === 'true' || value === '1' || value === 1;
      case 'string':
        return String(value);
      case 'array':
        if (Array.isArray(value)) return value;
        try {
          const parsed = JSON.parse(value);
          return Array.isArray(parsed) ? parsed : [];
        } catch {
          return [];
        }
      default:
        return value;
    }
  };

  // Load JSON file
  const handleFileChange = (e) => {
    const file = e.target.files[0];
    setFilename(file.name);
    const reader = new FileReader();
    reader.onload = (evt) => {
      try {
        const json = JSON.parse(evt.target.result);
        setData(json);
        setError("");
      } catch (err) {
        alert("Invalid JSON file");
      }
    };
    reader.readAsText(file);
  };

  // Handle cell edit for object
  const handleObjectChange = (key, value, originalValue) => {
    let newValue = value;
    const expectedType = propertyTypes[key];
    
    if (expectedType === 'array') {
      try {
        newValue = JSON.parse(value);
        if (!Array.isArray(newValue)) {
          setError(`Invalid array format for key '${key}': Expected array`);
          return;
        }
      } catch (e) {
        setError(`Invalid JSON for key '${key}': ${e.message}`);
        return;
      }
    } else if (expectedType) {
      newValue = convertToType(value, expectedType);
    } else if (typeof originalValue === 'object') {
      try {
        newValue = JSON.parse(value);
        setError("");
      } catch (e) {
        setError(`Invalid JSON for key '${key}': ${e.message}`);
        return;
      }
    }
    
    setError("");
    setData({ ...data, [key]: newValue });
  };

  // Handle cell edit for 2D array
  const handleArrayChange = (rowIdx, colIdx, value) => {
    const newData = data.map((row, r) =>
      row.map((cell, c) => (r === rowIdx && c === colIdx ? value : cell))
    );
    setData(newData);
  };

  // Download JSON
  const handleSave = () => {
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename || "chSettings.json";
    a.click();
    URL.revokeObjectURL(url);
  };

  // Render object as table
  const renderObjectTable = (obj) => (
    <table border="1" cellPadding="5">
      <thead>
        <tr><th>Key</th><th>Value</th></tr>
      </thead>
      <tbody>
        {Object.entries(obj).map(([key, value]) => (
          <tr key={key}>
            <td>{key}</td>
            <td>
              <input
                type="text"
                value={typeof value === 'object' ? JSON.stringify(value, null, 2) : value}
                onChange={e => handleObjectChange(key, e.target.value, value)}
                style={{ width: 300 }}
              />
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  );

  // Render 2D array as table
  const renderArrayTable = (arr) => (
    <table border="1" cellPadding="5">
      <tbody>
        {arr.map((row, rowIdx) => (
          <tr key={rowIdx}>
            {row.map((cell, colIdx) => (
              <td key={colIdx}>
                <input
                  type="text"
                  value={cell}
                  onChange={e => handleArrayChange(rowIdx, colIdx, e.target.value)}
                  style={{ width: 60 }}
                />
              </td>
            ))}
          </tr>
        ))}
      </tbody>
    </table>
  );

  // Get display value for input field
  const getDisplayValue = (value, type) => {
    if (value === null || value === undefined) return '';
    if (type === 'array') {
      return JSON.stringify(value);
    }
    return String(value);
  };

  // Get input type for HTML input
  const getInputType = (type) => {
    switch (type) {
      case 'integer':
      case 'float':
        return 'number';
      case 'boolean':
        return 'checkbox';
      default:
        return 'text';
    }
  };

  // Render all channels in a selected module (editable, horizontal, Material UI)
  const renderAllChannelsInModule = (arr) => {
    if (!Array.isArray(arr) || arr.length === 0) return null;
    const module = arr[selectedModule];
    if (!Array.isArray(module) || module.length === 0) return <Typography>No channels in selected module.</Typography>;
    const allKeys = Array.from(new Set(module.flatMap(ch => typeof ch === 'object' && ch !== null ? Object.keys(ch) : [])));
    const handleChannelValueChange = (channelIdx, key, value, originalValue) => {
      let newValue = value;
      const expectedType = propertyTypes[key];
      
      if (expectedType === 'array') {
        try {
          newValue = JSON.parse(value);
          if (!Array.isArray(newValue)) {
            setError(`Invalid array format for channel ${channelIdx}, key '${key}': Expected array`);
            return;
          }
        } catch (e) {
          setError(`Invalid JSON for channel ${channelIdx}, key '${key}': ${e.message}`);
          return;
        }
      } else if (expectedType) {
        newValue = convertToType(value, expectedType);
      } else if (typeof originalValue === 'object') {
        try {
          newValue = JSON.parse(value);
        } catch (e) {
          setError(`Invalid JSON for channel ${channelIdx}, key '${key}': ${e.message}`);
          return;
        }
      }
      
      setError("");
      const newData = data.map((mod, mIdx) =>
        mIdx === selectedModule
          ? mod.map((ch, cIdx) =>
              cIdx === channelIdx ? { ...ch, [key]: newValue } : ch
            )
          : mod
      );
      setData(newData);
    };

    return (
      <Box sx={{ margin: '10px 0', padding: 2, border: '1px solid #ccc', background: '#f5f5f5', overflowX: 'auto' }}>
        <Typography variant="h6" gutterBottom>All Channels in Module {selectedModule}:</Typography>
        <TableContainer component={Paper} sx={{ minWidth: 1200, maxHeight: 600 }}>
          <Table stickyHeader size="small">
            <TableHead>
              <TableRow>
                <TableCell sx={{ position: 'sticky', left: 0, background: '#f5f5f5', zIndex: 2 }}>Key</TableCell>
                {module.map((_, idx) => (
                  <TableCell key={idx}>Channel {idx}</TableCell>
                ))}
              </TableRow>
            </TableHead>
            <TableBody>
              {allKeys.map((k) => (
                <TableRow key={k}>
                  <TableCell sx={{ position: 'sticky', left: 0, background: '#f5f5f5', zIndex: 1 }}>
                    {k}
                    <br />
                    <Typography variant="caption" color="text.secondary">
                      ({propertyTypes[k] || 'unknown'})
                    </Typography>
                  </TableCell>
                  {module.map((channel, idx) => (
                    <TableCell key={idx}>
                      {typeof channel === 'object' && channel !== null && k in channel ? (
                        propertyTypes[k] === 'boolean' ? (
                          <input
                            type="checkbox"
                            checked={Boolean(channel[k])}
                            onChange={e => handleChannelValueChange(idx, k, e.target.checked, channel[k])}
                            style={{ transform: 'scale(1.2)' }}
                          />
                        ) : (
                          <TextField
                            size="small"
                            type={getInputType(propertyTypes[k])}
                            value={getDisplayValue(channel[k], propertyTypes[k])}
                            onChange={e => handleChannelValueChange(idx, k, e.target.value, channel[k])}
                            inputProps={{
                              step: propertyTypes[k] === 'float' ? 'any' : undefined,
                              min: propertyTypes[k] === 'integer' ? 0 : undefined
                            }}
                            sx={{ width: 180, height: 28, '& .MuiInputBase-input': { padding: '4px 8px', height: '20px' } }}
                          />
                        )
                      ) : ''}
                    </TableCell>
                  ))}
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      </Box>
    );
  };

  return (
    <Box sx={{ padding: 3 }}>
      <Typography variant="h4" gutterBottom>chSettings.json Editor</Typography>
      <input type="file" accept="application/json" onChange={handleFileChange} />
      {error && <Alert severity="error" sx={{ mt: 2 }}>{error}</Alert>}
      {data && (
        <Box sx={{ mt: 3 }}>
          {Array.isArray(data) && data.length > 0 && Array.isArray(data[0]) && (
            <>
              <Box sx={{ mb: 2 }}>
                <FormControl sx={{ minWidth: 120 }} size="small">
                  <InputLabel id="module-select-label">Module</InputLabel>
                  <Select
                    labelId="module-select-label"
                    value={selectedModule}
                    label="Module"
                    onChange={e => setSelectedModule(Number(e.target.value))}
                  >
                    {data.map((mod, idx) => (
                      <MenuItem key={idx} value={idx}>Module {idx}</MenuItem>
                    ))}
                  </Select>
                </FormControl>
              </Box>
              {renderAllChannelsInModule(data)}
            </>
          )}
          {/* Only show object table for non-array data */}
          {!Array.isArray(data) && renderObjectTable(data)}
          <Button variant="contained" color="primary" onClick={handleSave} sx={{ mt: 2 }}>Save JSON</Button>
        </Box>
      )}
    </Box>
  );
}

export default App;
