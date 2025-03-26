import axios from 'axios';

const API_BASE_URL = 'http://localhost:8080/api';

/**
 * Runs the benchmark by sending a POST request to the server.
 *
 * @param {number} numThreads - The number of threads for the benchmark.
 * @param {number} requestsPerThread - The number of requests per thread.
 * @param {function} onProgress - A callback function to update progress.
 *                                This function should accept a single parameter (the percent complete).
 * @returns {Promise<Object>} The benchmark results as a JSON object.
 */
export const runBenchmark = async (numThreads, requestsPerThread, onProgress) => {
  try {
    const response = await axios.post(
      `${API_BASE_URL}/benchmark`,
      {
        num_threads: numThreads,
        requests_per_thread: requestsPerThread,
      },
      {
        // onDownloadProgress is used to update progress as data is received.
        onDownloadProgress: progressEvent => {
          if (progressEvent.total) {
            // Calculate the percent completed
            let percentCompleted = Math.round(
              (progressEvent.loaded * 100) / progressEvent.total
            );
            // Log for debugging purposes
            console.log('Progress event: ' + percentCompleted + '%');
            // Call the onProgress callback with the current percent
            onProgress(percentCompleted);
          }
        },
      }
    );
    // Ensure progress is set to 100% when the request completes
    onProgress(100);
    return response.data;
  } catch (error) {
    // Even if there's an error, force progress to 100% so the UI can update
    onProgress(100);
    throw new Error(
      error.response?.data?.error || 'Failed to run benchmark'
    );
  }
};
