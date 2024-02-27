#define button 15 // Define the button pin
#define led 2 // Define the LED pin

void setup() 
{
  delay(500); // Wait for 0,5 seconds
  pinMode(button, INPUT); // Set the button pin as input
  pinMode(led, OUTPUT); // Set the LED pin as output
  Serial.begin(115200); // Initialize serial communication at 115200 bits per second
}

void loop() 
{
  int buttonState = analogRead(button); // Read the button state
  if (buttonState > 4000) // If the button is pressed
  {
    digitalWrite(led, HIGH); // Turn on the LED
  }
  else // If the button is not pressed
  {
    digitalWrite(led, LOW); // Turn off the LED
  }
}