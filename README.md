# LVGL-V8
    import RPi.GPIO as GPIO
    import time
    
    # Define GPIO pins for L298N driver and limit switches
    IN1 = 26
    IN2 = 16
    IN3 = 6
    IN4 = 5
    
    # Set GPIO mode and configure pins
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(IN1, GPIO.OUT)
    GPIO.setup(IN2, GPIO.OUT)
    GPIO.setup(IN3, GPIO.OUT)
    GPIO.setup(IN4, GPIO.OUT)
    
    # Define constants
    DEG_PER_STEP = 1.8
    STEPS_PER_REVOLUTION = int(360 / DEG_PER_STEP)
    
    # Global variables to store the previous state of the limit switches
    prev_limit_switch_1 = GPIO.HIGH
    prev_limit_switch_2 = GPIO.HIGH
    
    # Function to move the stepper motor one step forward
    def step_forward(delay):
        GPIO.output(IN1, GPIO.HIGH)
        GPIO.output(IN2, GPIO.HIGH)
        GPIO.output(IN3, GPIO.LOW)
        GPIO.output(IN4, GPIO.LOW)
        time.sleep(delay)
            
        GPIO.output(IN1, GPIO.LOW)
        GPIO.output(IN2, GPIO.HIGH)
        GPIO.output(IN3, GPIO.HIGH)
        GPIO.output(IN4, GPIO.LOW)
        time.sleep(delay)
    
    # Function to move the stepper motor one step backward
    def step_backward(delay):
        GPIO.output(IN1, GPIO.LOW)
        GPIO.output(IN2, GPIO.LOW)
        GPIO.output(IN3, GPIO.HIGH)
        GPIO.output(IN4, GPIO.HIGH)
        time.sleep(delay)
            
        GPIO.output(IN1, GPIO.HIGH)
        GPIO.output(IN2, GPIO.LOW)
        GPIO.output(IN3, GPIO.LOW)
        GPIO.output(IN4, GPIO.HIGH)
        time.sleep(delay)
    
    try:
        # Set the delay between steps
        delay = 0.01
    
        # Set the initial direction
        direction = 'forward'
    
        while True:
            # Move the stepper motor based on the direction
            direction == 'forward'
            step_forward(delay)
            direction == 'backward'
            step_backward(delay)
    
    except KeyboardInterrupt:
        print("\nExiting the script.")
    
    finally:
        # Clean up GPIO settings
        GPIO.cleanup()
