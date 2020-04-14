#Compiling
First, you should make sure that all of the files in this directory are in the same directory.
Then, go to your red hat linux 2.4 machine(the files should be there) and compile with make, the output file will be test

#Using
You can run the program with ./test.
when running the program, you ,may enter those commands:
#1)create_list:
create_list will create an empty list
#2)delete_list:
delete the current linked-list, as well as all of the data stored inside
#3)print_list:
print the list elements
#4)insert_value#value:
adding a new value to the list(the elements in the list are sorted) that has the value #value
#5)remove_value#value:
removing the first element in the list that has the value #value
#6)count_greater#predict:
counting and printing how many numbers in the list are greater than #predict
#7)join:
make the main thread waiting to all of the other threads(each command will open a new thread and run there,
the whole point of the exercise is to program a multy-treading spported linked-list)
#8)exit:
will end the running, you may assume that delete_list always called before exit.


#You cannot just lock the whole list in any command, you should lock only the relevant part of the list for your command
(for example remove_value 80 shouldn't lock insert_value 10, but should lock remove_value 79) 
