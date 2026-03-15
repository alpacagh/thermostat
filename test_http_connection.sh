#!/bin/bash

# HTTP Connection Stability Test Script
# Tests the thermostat's HTTP connection stability over time

THERMOSTAT_IP="192.168.1.100"  # Update with your thermostat IP
TEST_DURATION=300  # 5 minutes test
INTERVAL=10  # Test every 10 seconds
LOG_FILE="http_connection_test.log"

echo "Starting HTTP Connection Stability Test..."
echo "Target: $THERMOSTAT_IP"
echo "Duration: $TEST_DURATION seconds"
echo "Interval: $INTERVAL seconds"
echo "Log file: $LOG_FILE"
echo "----------------------------------------"

# Initialize log
echo "$(date): Starting HTTP connection stability test" > $LOG_FILE

success_count=0
failure_count=0
start_time=$(date +%s)

while [ $(($(date +%s) - start_time)) -lt $TEST_DURATION ]; do
    timestamp=$(date +"%Y-%m-%d %H:%M:%S")
    
    # Test HTTP connection
    if curl -s --max-time 10 "http://$THERMOSTAT_IP/api/status" > /dev/null 2>&1; then
        echo "$timestamp: SUCCESS - HTTP request completed"
        echo "$timestamp: SUCCESS" >> $LOG_FILE
        ((success_count++))
    else
        echo "$timestamp: FAILURE - HTTP request failed"
        echo "$timestamp: FAILURE" >> $LOG_FILE
        ((failure_count++))
    fi
    
    sleep $INTERVAL
done

# Summary
end_time=$(date +%s)
total_tests=$((success_count + failure_count))
success_rate=$((success_count * 100 / total_tests))

echo "----------------------------------------"
echo "Test Summary:"
echo "Total tests: $total_tests"
echo "Successful: $success_count"
echo "Failed: $failure_count"
echo "Success rate: $success_rate%"
echo "----------------------------------------"

# Save summary to log
echo "----------------------------------------" >> $LOG_FILE
echo "Test Summary:" >> $LOG_FILE
echo "Total tests: $total_tests" >> $LOG_FILE
echo "Successful: $success_count" >> $LOG_FILE
echo "Failed: $failure_count" >> $LOG_FILE
echo "Success rate: $success_rate%" >> $LOG_FILE

if [ $success_rate -ge 95 ]; then
    echo "✅ PASS: Connection stability is excellent"
    exit 0
elif [ $success_rate -ge 80 ]; then
    echo "⚠️  WARN: Connection stability is acceptable but could be improved"
    exit 1
else
    echo "❌ FAIL: Connection stability is poor"
    exit 2
fi