/*To generate the object file for m3 arm architecture use :

arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb <the file > -o <the output file .o>  */

/*to add debug information  use :
arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb --debug <the file > -o <the output file .o>  */

*/



/* to analysis the opject code and see the lookup table use : 

arm-none-eabi-objdump -t <filename .o>
*/

/*to use the linker and generate map file :

arm-none-eabi-ld -Map application.map -T LinkerScript.ld startup_stm32f103.o -o startup_stm32f103.elf
*/

/* to convert the .elf to binary 

arm-none-eabi-objcopy -O binary startup_stm32f103.elf  startup_stm32f103.bin
*/



