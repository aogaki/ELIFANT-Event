import React, { useState, useRef } from 'react';
import { Box, Typography } from '@mui/material';

// Simple test component to debug input issues
function InputTest() {
  const [value, setValue] = useState('');
  const [logs, setLogs] = useState([]);
  const inputRef = useRef(null);

  const addLog = (message) => {
    setLogs(prev => [...prev, `${new Date().toLocaleTimeString()}: ${message}`]);
  };

  return (
    <Box sx={{ padding: 2, border: '1px solid #ccc', margin: 2 }}>
      <Typography variant="h6">Input Test Component</Typography>
      <Typography variant="body2" color="text.secondary">
        Test typing commas and spaces here. If this works, the issue is in the main component.
      </Typography>
      
      {/* Controlled input - like what we're trying to fix */}
      <div style={{ marginBottom: '10px' }}>
        <Typography variant="body2">Controlled Input (React managed):</Typography>
        <input
          type="text"
          value={value}
          onChange={(e) => {
            addLog(`Controlled onChange: "${e.target.value}"`);
            setValue(e.target.value);
          }}
          onKeyDown={(e) => {
            addLog(`Controlled onKeyDown: key="${e.key}" code=${e.keyCode}`);
          }}
          placeholder="Type comma, space, and letters here"
          style={{
            width: '300px',
            height: '30px',
            padding: '5px',
            border: '1px solid #ccc',
            borderRadius: '4px',
            fontSize: '14px',
            margin: '5px 0',
            backgroundColor: '#fff'
          }}
        />
      </div>

      {/* Uncontrolled input - like the working debug page */}
      <div style={{ marginBottom: '10px' }}>
        <Typography variant="body2">Uncontrolled Input (DOM managed):</Typography>
        <input
          ref={inputRef}
          type="text"
          defaultValue=""
          onChange={(e) => {
            addLog(`Uncontrolled onChange: "${e.target.value}"`);
          }}
          onKeyDown={(e) => {
            addLog(`Uncontrolled onKeyDown: key="${e.key}" code=${e.keyCode}`);
          }}
          placeholder="Type comma, space, and letters here"
          style={{
            width: '300px',
            height: '30px',
            padding: '5px',
            border: '1px solid #ccc',
            borderRadius: '4px',
            fontSize: '14px',
            margin: '5px 0',
            backgroundColor: '#fff'
          }}
        />
      </div>
      
      <Typography variant="body2">
        Controlled value: "{value}"
      </Typography>
      
      <Typography variant="body2" sx={{ mt: 1 }}>
        Test these characters: <strong>, (comma)</strong> and <strong>(space)</strong>
      </Typography>
      
      <Box sx={{ maxHeight: '200px', overflowY: 'auto', backgroundColor: '#f5f5f5', padding: 1, mt: 1 }}>
        <Typography variant="caption">Event Log:</Typography>
        {logs.map((log, index) => (
          <div key={index} style={{ fontSize: '12px', fontFamily: 'monospace' }}>
            {log}
          </div>
        ))}
      </Box>
      
      <button onClick={() => setLogs([])}>Clear Log</button>
    </Box>
  );
}

export default InputTest;
