import React from 'react';
import {
    Paper,
    Typography,
    Grid,
    Box,
    Table,
    TableBody,
    TableCell,
    TableContainer,
    TableHead,
    TableRow
} from '@mui/material';
import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend
} from 'chart.js';
import { Line } from 'react-chartjs-2';

ChartJS.register(
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend
);

const ResultsDisplay = ({ results }) => {
    if (!results) return null;

    const latencyData = {
        labels: ['Average', 'P50', 'P95', 'P99'],
        datasets: [
            {
                label: 'Latency (ms)',
                data: [
                    results.avg_latency,
                    results.p50_latency,
                    results.p95_latency,
                    results.p99_latency
                ],
                backgroundColor: 'rgba(54, 162, 235, 0.5)',
                borderColor: 'rgba(54, 162, 235, 1)',
                borderWidth: 1
            }
        ]
    };

    const options = {
        responsive: true,
        plugins: {
            legend: {
                position: 'top',
            },
            title: {
                display: true,
                text: 'Latency Distribution'
            }
        },
        scales: {
            y: {
                beginAtZero: true,
                title: {
                    display: true,
                    text: 'Latency (ms)'
                }
            }
        }
    };

    return (
        <Paper elevation={3} sx={{ p: 3 }}>
            <Typography variant="h5" gutterBottom>
                Benchmark Results
            </Typography>
            
            <Grid container spacing={3}>
                <Grid item xs={12} md={6}>
                    <Box sx={{ height: 300 }}>
                        <Line data={latencyData} options={options} />
                    </Box>
                </Grid>
                
                <Grid item xs={12} md={6}>
                    <TableContainer>
                        <Table>
                            <TableBody>
                                <TableRow>
                                    <TableCell component="th" scope="row">Total Requests</TableCell>
                                    <TableCell align="right">{results.total_requests}</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">Failed Requests</TableCell>
                                    <TableCell align="right">{results.failed_requests}</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">Throughput</TableCell>
                                    <TableCell align="right">{results.throughput.toFixed(2)} req/s</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">Average Latency</TableCell>
                                    <TableCell align="right">{results.avg_latency.toFixed(2)} ms</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">P50 Latency</TableCell>
                                    <TableCell align="right">{results.p50_latency.toFixed(2)} ms</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">P95 Latency</TableCell>
                                    <TableCell align="right">{results.p95_latency.toFixed(2)} ms</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">P99 Latency</TableCell>
                                    <TableCell align="right">{results.p99_latency.toFixed(2)} ms</TableCell>
                                </TableRow>
                                <TableRow>
                                    <TableCell component="th" scope="row">Duration</TableCell>
                                    <TableCell align="right">{results.duration.toFixed(2)} s</TableCell>
                                </TableRow>
                            </TableBody>
                        </Table>
                    </TableContainer>
                </Grid>
            </Grid>
        </Paper>
    );
};

export default ResultsDisplay; 