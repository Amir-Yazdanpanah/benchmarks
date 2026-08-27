# Compiling the baremetal code and writing to SD card

The Edge Detection source code available in this directory targets baremetal flow for RISC-V.

image_data.h, image_data1.h and boat.h are three images in C byte array format. 

boat.h uses one-dimensional array for ease of use; eliminating the necessity of an additional loop in C-code.

Former two files use two-dimensional array. Boat.h is recommended.

If image_data.h and image_data1.h are to be processed, please uncomment section 1 in the main() function of edgeDetect_cm.C code. 

Define the WIDTH and HEIGHT of the used image in the C-code accordingly.

The accompanying Python script waits for a marker (START_EDGES) from RISC-V and after it is received, it starts counting the bytes and verifies that the correct image data is received. It then goes on to reconstruct the detected edges and displays and saves the image. This step was included just for verification. If this is not required, keep section 5 commented and do no use the Python file. Instead use a serial terminal on host PC (for example at ttyUSB0 with baudrate 115200). The example output is shown at the end. When UART is being read, Python script will throw and error that the device is either disconnected or has multiple access on port!



After uploading the bitstream, do the following:
* Plug the SD card in the card reader on your PC
* Change the SD card name in autoEdge.sh script (of=/dev/sdd)
* Run the script (./autoEdge.sh)
* The compiled code will be written to sector 34 of the SD card
* This is the default location Rocket Core will read at the boot-up
* Put the SD card in the card slot on VC707
* Press SW8 to re-read the SD card and boot the RISC-V up

## Example output on UART without using Python script

> INIT                                                                            
> CMD0                                                                            
> CMD8                                                                            
> ACMD41                                                                          
> CMD58                                                                           
> CMD16                                                                           
> CMD18                                                                           
> LOADING 0x01e00000 B PAYLOAD                                                    
> LOADING                                                                         
> BOOT                                                                            
> Boot ROM running                                                                
>                 [INFO] Starting edge detection...                               
> Edge detection finished
