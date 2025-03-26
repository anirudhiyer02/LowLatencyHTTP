import React from 'react';
import { Box, LinearProgress, Typography, Paper } from '@mui/material';

const ProgressIndicator = ({ isRunning, progress }) => {
    if (!isRunning) return null;

    return (
        <Paper elevation={3} sx={{ p: 3, mb: 3 }}>
            <Typography variant="h6" gutterBottom>
                Running Benchmark
            </Typography>
            <Box sx={{ width: '100%' }}>
                <LinearProgress 
                    variant="determinate" 
                    value={progress} 
                    sx={{ height: 10, borderRadius: 5 }}
                />
                <Typography variant="body2" color="text.secondary" sx={{ mt: 1 }}>
                    {progress}% Complete
                </Typography>
            </Box>
        </Paper>
    );
};

export default ProgressIndicator; 