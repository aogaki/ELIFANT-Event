// Test script to verify type conversions
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

// Test cases
console.log('Testing type conversions:');
console.log('Integer "123" ->', convertToType('123', 'integer')); // Should be 123
console.log('Float "3.14" ->', convertToType('3.14', 'float')); // Should be 3.14
console.log('Boolean "true" ->', convertToType('true', 'boolean')); // Should be true
console.log('Boolean "false" ->', convertToType('false', 'boolean')); // Should be false
console.log('String 123 ->', convertToType(123, 'string')); // Should be "123"
console.log('Array "[]" ->', convertToType('[]', 'array')); // Should be []
console.log('Array "[\"E_Sector\"]" ->', convertToType('["E_Sector"]', 'array')); // Should be ["E_Sector"]
