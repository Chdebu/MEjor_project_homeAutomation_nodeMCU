# MEjor_project_homeAutomation_nodeMCU
(Each relay is active LOW, meaning LOW = ON)

Relay No.	GPIO Pin	NodeMCU Label	Description
Relay 1	GPIO5	D1	Connected to relay IN1
Relay 2	GPIO4	D2	Connected to relay IN2
Relay 3	GPIO12	D6	Connected to relay IN3
Relay 4	GPIO13	D7	Connected to relay IN4

Make sure relay module GND is connected to NodeMCU GND.

🔘 Button Connections
(Each button is connected between GPIO and GND)

Button No.	GPIO Pin	NodeMCU Label	Description
Button 1	GPIO14	D5	Push-button to toggle Relay 1
Button 2	GPIO15	D8	Push-button to toggle Relay 2
Button 3	GPIO3	RX	Push-button to toggle Relay 3 (use caution)
Button 4	GPIO1	TX	Push-button to toggle Relay 4 (use caution)

⚠️ Note: RX (GPIO3) and TX (GPIO1) are serial pins. Using them for buttons may interfere with USB communication. It's better to use other GPIOs like GPIO0 (D3), GPIO2 (D4) if available.