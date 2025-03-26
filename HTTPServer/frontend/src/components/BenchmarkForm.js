import React, { useState } from 'react';
import { 
    Box, 
    TextField, 
    Button, 
    Paper, 
    Typography,
    Slider,
    FormControl,
    InputLabel,
    Select,
    MenuItem
} from '@mui/material';

const BenchmarkForm = ({ onSubmit, isRunning }) => {
    const [config, setConfig] = useState({
        numThreads: 4,
        requestsPerThread: 1000
    });

    const handleSubmit = (e) => {
        e.preventDefault();
        onSubmit(config);
    };

    const handleChange = (field) => (event) => {
        setConfig(prev => ({
            ...prev,
            [field]: event.target.value
        }));
    };

    return (
        <Paper elevation={3} sx={{ p: 3, mb: 3 }}>
            <Typography variant="h5" gutterBottom>
                Benchmark Configuration
            </Typography>
            <Box component="form" onSubmit={handleSubmit} sx={{ mt: 2 }}>
                <FormControl fullWidth sx={{ mb: 2 }}>
                    <InputLabel>Number of Threads</InputLabel>
                    <Select
                        value={config.numThreads}
                        label="Number of Threads"
                        onChange={handleChange('numThreads')}
                        disabled={isRunning}
                    >
                        {[1, 2, 4, 8, 16, 32, 64, 100].map((num) => (
                            <MenuItem key={num} value={num}>
                                {num}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>

                <FormControl fullWidth sx={{ mb: 2 }}>
                    <InputLabel>Requests per Thread</InputLabel>
                    <Select
                        value={config.requestsPerThread}
                        label="Requests per Thread"
                        onChange={handleChange('requestsPerThread')}
                        disabled={isRunning}
                    >
                        {[100, 500, 1000, 2000, 5000, 10000].map((num) => (
                            <MenuItem key={num} value={num}>
                                {num}
                            </MenuItem>
                        ))}
                    </Select>
                </FormControl>

                <Button
                    type="submit"
                    variant="contained"
                    color="primary"
                    fullWidth
                    disabled={isRunning}
                    sx={{ mt: 2 }}
                >
                    {isRunning ? 'Running...' : 'Run Benchmark'}
                </Button>
            </Box>
        </Paper>
    );
};

export default BenchmarkForm; 