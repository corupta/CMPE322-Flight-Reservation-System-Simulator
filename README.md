# CMPE322-Flight-Reservation-System-Simulator
#### Simulating a Flight Reservation System with multiple threaded programming

## Design and Methodology





## How to Compile
```
gcc project2.c
```
(doing so would create an executable called `a.out`)

or

```
gcc project2.c -o <program_name>
```
(doing so would create an executable called `program_name`)

or
```
make
```
(doing so would execute `gcc project2.c -o flightReservationSystemSimulator` behind the scenes, creating you an executable called `flightReservationSystemSimulator`)

## How to Run

```
./flightReservationSystemSimulator <number_of_clients>
```
(number of clients must be an integer between 50 and 100 where both are inclusive)

Doing so would simulate a flight reservation system as explained in the 
[Project Description](./Project2.pdf) with `number_of_clients` clients and the same number of seats outputting the results into `output.txt` file. 


* Also, the output should consist of `<number_of_clients> + 2` lines where
* The first line is `Number of total seats: <number_of_clients>`,
* The lines `2` to `<number_of_clients> + 1` are which client reserved which seat
* The last line (`<number_of_clients> + 2`th one) is `All seats are reserved.`
* The output ends with an end line as is usual and expected in linux environments.
(`<number_of_endlines` = `<number_of_lines>`  = `<number_of_clients> + 2`)

   

### Example:

Running,
```
./flightReservationSystemSimulation 56
```
creates the below `output.txt` file:

![SampleOutput](./SampleOutput.png)



