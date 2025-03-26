import React, { useState } from 'react';
import { Container, Typography, Box, Alert } from '@mui/material';
import BenchmarkForm from './components/BenchmarkForm';
import ResultsDisplay from './components/ResultsDisplay';
import ProgressIndicator from './components/ProgressIndicator';
import { runBenchmark } from './services/api';

function App() {
    const [isRunning, setIsRunning] = useState(false);
    const [results, setResults] = useState(null);
    const [error, setError] = useState(null);
    const [progress, setProgress] = useState(0);

    const handleSubmit = async (config) => {
        setIsRunning(true);
        setError(null);
        setProgress(0);
        setResults(null);

        try {
            // Simulate progress updates
            const progressInterval = setInterval(() => {
                setProgress(prev => {
                    if (prev >= 90) {
                        clearInterval(progressInterval);
                        return prev;
                    }
                    return prev + 10;
                });
            }, 1000);

            const results = await runBenchmark(config.numThreads, config.requestsPerThread, ()=> {});
            clearInterval(progressInterval);
            setProgress(100);
            setResults(results);
        } catch (err) {
            setError(err.message);
        } finally {
            setIsRunning(false);
            setProgress(0);
        }
    };

    return (
        <Container maxWidth="lg" sx={{ py: 4 }}>
            <Typography variant="h4" component="h1" gutterBottom align="center">
                HTTP Client Benchmark
            </Typography>

            {error && (
                <Alert severity="error" sx={{ mb: 3 }}>
                    {error}
                </Alert>
            )}

            <Box sx={{ mb: 4 }}>
                <BenchmarkForm onSubmit={handleSubmit} isRunning={isRunning} />
            </Box>

            <ProgressIndicator isRunning={isRunning} progress={progress} />

            {results && (
                <Box sx={{ mt: 4 }}>
                    <ResultsDisplay results={results} />
                </Box>
            )}
        </Container>
    );
}

export default App;
