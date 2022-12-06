# FreeRTOS
FreeRTOS projects an ARM M3-based development board (NXP LPC1549).

<h2>Programs:</h2>
<p><a href= https://github.com/jaakkoiot/FreeRTOS/tree/main/min_sec_UART_print>Time printer</a> prints the time the program has run in min:sec -format on UART</p>
<p><a href= https://github.com/jaakkoiot/FreeRTOS/tree/main/visual_SOS_morser>SOS morser</a> toggles the onboard red LED in a dot - dash - dot fashion with green LED accenting every other sequence</p>
<p><a href= https://github.com/jaakkoiot/FreeRTOS/tree/main/variable_incrementer>Variable incrementer</a> increments a count that it prints on UART either 1/sec if no button is pressed or 10/sec if SW1 is pressed</p>
<p><a href= https://github.com/jaakkoiot/FreeRTOS/tree/main/mutex_printer>Mutex guard</a> for serial string printing -> SW1-3 print their info on the terminal</p>
<p><a href= https://github.com/jaakkoiot/FreeRTOS/tree/main/semaphore_activity_indicator>Activity indicator</a> using a binary semaphore to illuminate onboard LED when UART is written to
<p><a href=https://github.com/jaakkoiot/FreeRTOS/tree/main/semaphore_oracle>Oracle</a> answers questions through ITM terminal. Selects randomnly from a pool of predetermined answers.</p>
