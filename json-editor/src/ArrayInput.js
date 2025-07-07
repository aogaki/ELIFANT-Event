import React, { useRef, useEffect } from 'react';

const ArrayInput = ({ value, onChange, placeholder, style }) => {
  const inputRef = useRef(null);

  // Convert array to display string
  const getDisplayValue = (arrayValue) => {
    if (!Array.isArray(arrayValue)) return '';
    if (arrayValue.length === 0) return '';
    // Show as comma-separated values for better UX
    return arrayValue.join(', ');
  };

  // Parse input string to array
  const parseToArray = (inputValue) => {
    if (!inputValue || inputValue.trim() === '') return [];
    
    // If it looks like JSON, try to parse as JSON
    if (inputValue.trim().startsWith('[') && inputValue.trim().endsWith(']')) {
      try {
        const parsed = JSON.parse(inputValue);
        return Array.isArray(parsed) ? parsed : [];
      } catch (e) {
        throw new Error('Invalid JSON format');
      }
    }
    
    // Otherwise, treat as comma-separated values
    return inputValue.split(',').map(item => item.trim()).filter(item => item.length > 0);
  };

  // Update input value when prop changes (but only if not focused)
  useEffect(() => {
    if (inputRef.current && document.activeElement !== inputRef.current) {
      inputRef.current.value = getDisplayValue(value);
    }
  }, [value]);

  const handleChange = (e) => {
    const newValue = e.target.value;
    console.log('ArrayInput handleChange:', newValue);
    
    try {
      const arrayValue = parseToArray(newValue);
      onChange(arrayValue);
    } catch (error) {
      console.error('Parse error:', error.message);
      // Don't call onChange if parsing fails, but let user continue typing
    }
  };

  return (
    <input
      ref={inputRef}
      type="text"
      defaultValue={getDisplayValue(value)}
      onChange={handleChange}
      onKeyDown={(e) => {
        // Log all key events for debugging
        console.log('ArrayInput keydown:', e.key, e.keyCode, e.target.value);
        
        // Don't prevent any keys - let everything through
      }}
      onKeyPress={(e) => {
        console.log('ArrayInput keypress:', e.key, e.charCode, e.target.value);
      }}
      onInput={(e) => {
        console.log('ArrayInput input:', e.target.value);
      }}
      placeholder={placeholder || "Type comma-separated values"}
      style={{
        width: '175px',
        height: '24px',
        padding: '4px 8px',
        border: '1px solid #ccc',
        borderRadius: '4px',
        fontSize: '14px',
        fontFamily: 'monospace',
        backgroundColor: '#fff',
        ...style
      }}
    />
  );
};

export default ArrayInput;
