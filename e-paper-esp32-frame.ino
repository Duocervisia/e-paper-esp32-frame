#define button 12 // Define the button pin
#define led 2 // Define the LED pin

void setup() 
{
  // delay(500); // Wait for 0,5 seconds
  pinMode(button,INPUT_PULLUP);
  // pinMode(button, OUTPUT); // Set the button pin as input
  pinMode(led, OUTPUT); // Set the LED pin as output
  Serial.begin(115200); // Initialize serial communication at 115200 bits per second
}

void loop() 
{
  int buttonState = digitalRead(button); // Read the button state
  Serial.println(buttonState); // Print the button state
  if (buttonState == HIGH) // If the button is pressed
  {
    digitalWrite(led, LOW); // Turn on the LED
  }
  else // If the button is not pressed
  {
    digitalWrite(led, HIGH); // Turn off the LED
  }
}