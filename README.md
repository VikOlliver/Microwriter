# Microwriter
A reboot of the 80's Microwriter accessible chord keyboard done using an Arduino

The Microwriter and Quinkey were 6-key chord keyboards created in the 80's for use by people with various physical limitations such as brittle bones. They developed a following among all types of users being simple, reliable, easy to use, and effectively allowed instant touch typing at speed. See https://en.wikipedia.org/wiki/Microwriter

The chord sequence is vaguely mnemonic, but where the Microwriter/Quinkey patents have long expired, the copyright on the mnemonic instruction sheet has not. Versions can be found on the web regardless (most notably on Microsoft's website of all things). A new Beginner's Guide and a typing tutor can be found at https://github.com/VikOlliver/Quirkey

This version uses ATMega32u4 Arduino-compatible devices to scan the six keys and generate standard US USB HID keyboard and mouse (yes, mouse) input. Internal pullups are used, so beyond the microswitches and a USB lead no other components are needed.

A 3D-printable shell for the device can be found at https://www.thingiverse.com/thing:3433244

The original Microwriter used an RCA 1802 CPU and LCD for the user interface, and uploaded via RS232. A TV interface was also available. The author (Vik Olliver) created the original Amstrad and IBM Quinkey drivers.
