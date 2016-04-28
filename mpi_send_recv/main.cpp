#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdio.h>
#include <iomanip>
#include <mpi.h>
#include <stdlib.h>
#include <ctime>


using namespace std;

typedef vector<vector<char> > table;

const string standart_path = "/Users/Alexander/Documents/PROGA/1task/1task/test.csv";

const bool cluster = true;

void read_from_file(string name_of_file, table &field){
    ifstream file;
    file.open(name_of_file);
    while (!file.eof()){
        string temp_string;
        getline(file, temp_string);
        vector<char> temp_vector;
        for (int i = 0; i < temp_string.size(); i++){
            if (temp_string[i] == '0')
                temp_vector.push_back(0);
            else if (temp_string[i] == '1')
                temp_vector.push_back(1);
        }
        field.push_back(temp_vector);
    }
}

void generate_field(table &field){
    for (int i = 0; i < field.size(); i++)
        for (int j = 0; j < field[0].size(); j++)
            field[i][j] = rand() % 2;
}

void write_field(const table &field){
    for (int i = 0; i < field.size(); i++){
        for (int j = 0; j < field[0].size(); j++){
            if (field[i][j] == 0) cout << ' ';
            else cout << '1';
        }
        cout << "\n";
    }
}
//tested
void vector_to_chararray(const vector<char> &vec, char *array){
    for (int i = 0; i < vec.size(); i++)
        array[i] = vec[i];
}
void array_to_vector(const char* array, vector<char> &vec){
    for (int i = 0; i < vec.size(); i++)
        vec[i] = array[i];
}
// поле, начало и конец области(без запаса), куда писать
//tested
void matrix_to_array(table field, int start, int end, vector<char> &array){
    if (start == 0){
        for (int j = 0; j < field[0].size(); j++){
            array.push_back(field[field.size() - 1][j]);
        }
    }
    else start--;
    for (int i = start; i <= end; i++){
        for (int j = 0; j < field[0].size(); j++){
            array.push_back(field[i][j]);
        }
    }
    if (end == field.size() - 1)
        end = 0;
    else
        end++;
    for (int j = 0; j < field[0].size(); j++){
        array.push_back(field[end][j]);
    }
 
}

void table_to_array(table field, char *array){
    int count = 0;
    for (int i = 1; i < field.size() - 1; i++)
        for (int j = 0; j < field[0].size(); j++){
            array[count] = field[i][j];
            count++;
        }
}
//tested
void array_to_matrix(const char *array, int size_of_array, table &minifield, int len_of_string){
    minifield.assign(size_of_array / len_of_string, vector<char> (len_of_string, 0));
    for (int i = 0; i < size_of_array; i++){
        minifield[i / len_of_string][i % len_of_string] = array[i];
    }
}


void check_neighbors(int x, int y, const table &field1, table &field2){
    int num = 0;
    int n = field1.size();
    int m = field1[0].size();
    for (int i = - 1; i <= 1; i++)
        for (int j = -1; j<= 1; j++){
            int nx = x + i;
            int ny = y + j;
            if (nx >= n) nx = 0;
            else if (nx < 0) nx = n - 1;
            if (ny >= m) ny = 0;
            else if (ny < 0) ny = m - 1;
            if (field1[nx][ny] == 1 && (i != 0 || j != 0))
                num++;
        }
    if (num == 3)
        field2[x][y] = 1;
    else if (num == 2 && field1[x][y] == 1) field2[x][y] = 1;
    else field2[x][y] = 0;
}
// передаю на 1 процесс меньше
void parallel_king(table &field, int num_of_proc, int num_of_steps){
        //cout << "KING IN\n";
    int len_of_string = field[0].size();
    //cout << "WTF\n";
    vector<pair<int, int> > gran(num_of_proc);
    int last = 0;
    for (int i = 0; i < num_of_proc; i++){
        pair<int, int> temp;
        temp.first = last;
        last += field.size() / (num_of_proc);
        if (field.size() % num_of_proc > i) last++;
        temp.second = last - 1;
        gran[i] = temp;
    }
    
    for (int proc = 1; proc <= num_of_proc; proc++){
        MPI_Send(&num_of_steps, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
        //cout << "King send NUM_OF_STEPS " << num_of_steps << "to slave #" << proc << "\n";
        vector<char> arrayvec;
        matrix_to_array(field, gran[proc - 1].first, gran[proc - 1].second, arrayvec);
        int size_of_array = arrayvec.size();
        char array[size_of_array];
        vector_to_chararray(arrayvec, array);
        MPI_Send(&size_of_array, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
        //cout << "King send SIZE_OF_ARRAY " << size_of_array << "to slave #" << proc << "\n";
        MPI_Send(&array, size_of_array, MPI_CHAR, proc, 228, MPI_COMM_WORLD);
        //cout << "King send ARRAY " << sizeof(array)/sizeof(char) << "to slave #" << proc << "\n";

        int len_of_string = field[0].size();
        MPI_Send(&len_of_string, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
        //cout << "King send LEN_OF_STRING " << len_of_string << "to slave #" << proc << "\n";
    }
    //cout << "KING OUT\n";
}

void write_matrix(const table &field){
    for (int i = 0; i < field.size(); i++){
        for (int j = 0; j < field[0].size();j++){
            cout << (int)field[i][j];
        }
        cout << "\n";
    }
}

void parallel_slave(int rank){
    //cout << "SLAVE IN #" << rank << "\n";
    MPI_Request request;
    int stop_bit = 0;
    int num_of_steps;
    int size_of_array;
    int size_of_line;
    int num_of_proc;
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_proc);
    
    MPI_Recv(&num_of_steps, 1, MPI_INT, 0, 228, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //cout << "Slave #" << rank << " recv NUM_OF_STEPS " << num_of_steps << "\n";
    
    if (num_of_steps == 0) {
        MPI_Finalize();
      //  cout << "PROC #" << rank << " ended\n";
        exit(0);
    }
    
    MPI_Recv(&size_of_array, 1, MPI_INT, 0, 228, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //cout << "Slave #" << rank << " recv SIZE_OF_ARRAY " << size_of_array << "\n";
    char array[size_of_array];
    MPI_Recv(&array, size_of_array, MPI_CHAR, 0, 228, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
     //cout << "Slave #" << rank << " recv ARRAY " << sizeof(array)/sizeof(char) << "\n";
    //for (int i = 0; i < size_of_array; i++) cout << (int)array[i];
    //cout << "\n";
    MPI_Recv(&size_of_line, 1, MPI_INT, 0, 228, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //cout << "Slave #" << rank << " recv SIZE_OF_LINE " << size_of_line << "\n";
    //cout << "SLAVE #" << rank << " ALL INFORMATION RECIVED\n";
    table minifield;
    array_to_matrix(array, size_of_array, minifield, size_of_line);
    //cout << "Slave #" << rank << "part of field FIRST\n";
    //write_matrix(minifield);//debug
    //cout << "Slave #" << rank << " array converted\n";
    if (rank == 1){
        MPI_Irecv(&stop_bit, 1, MPI_INT, 0, 153, MPI_COMM_WORLD, &request);
    }
    else{
        MPI_Irecv(&num_of_steps, 1, MPI_INT, 1, 228, MPI_COMM_WORLD, &request);
    }
    
    for (int step = 0; step < num_of_steps; step++){
        if (rank == 1 && stop_bit == 1){
            //cout << "EVERYONE GET IN HERE\n";
            int total_num_of_steps = step + num_of_proc / 2 + 1;
            total_num_of_steps = min(total_num_of_steps, num_of_steps);
            for (int proc = 2; proc < num_of_proc; proc++){
                MPI_Ssend(&total_num_of_steps, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
                //cout << "Proc#" << proc << " sent information \n";
            }
            num_of_steps = total_num_of_steps;
            stop_bit = 0;
        }
        
        table tempfield = minifield;
        for (int i = 1; i < minifield.size() - 1; i++){
            for (int j = 0; j < minifield[0].size(); j++){
                check_neighbors(i, j, minifield, tempfield);
            }
        }
        //cout << "Slave #" << rank << "part of field SECOND\n";
        //write_matrix(tempfield);
        //cout << "Slave #" << rank << " guys checked\n";
        char line_to_send_to_up[size_of_line];
        char line_to_send_to_down[size_of_line];
        
        vector_to_chararray(tempfield[1], line_to_send_to_up);
        vector_to_chararray(tempfield[minifield.size() - 2], line_to_send_to_down);
        
        char line_to_recv_from_up[size_of_line];
        char line_to_recv_from_down[size_of_line];
        
        int rank_up = rank - 1;
        if (rank_up == 0)
            rank_up = num_of_proc - 1;
        int rank_down = rank + 1;
        if (rank_down == num_of_proc)
            rank_down = 1;
        
        MPI_Sendrecv(&line_to_send_to_down, size_of_line, MPI_CHAR, rank_down, 2, &line_to_recv_from_up, size_of_line, MPI_CHAR, rank_up, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&line_to_send_to_up, size_of_line, MPI_CHAR, rank_up, 2, &line_to_recv_from_down, size_of_line, MPI_CHAR, rank_down, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        array_to_vector(line_to_recv_from_down, tempfield[tempfield.size() - 1]);
        array_to_vector(line_to_recv_from_up, tempfield[0]);
        minifield = tempfield;
        //cout << "Slave #" << rank << "AFTER SWAP\n";
        //write_matrix(minifield);
    }
    //cout << "Node #" << rank << " finished on step " << num_of_steps << "\n";
    int temp_size = size_of_array - 2 * size_of_line;
    char temp[temp_size];
    table_to_array(minifield, temp);
    //cout << "Transfered array by Node#" << rank << " size =" << temp_size << "\n";
    //for (int i = 0; i < temp_size; i++)
    //    cout << (int)temp[i];
    //cout << "\n";
    MPI_Ssend(&temp_size, 1, MPI_INT, 0, 148, MPI_COMM_WORLD);
    MPI_Ssend(&temp, temp_size, MPI_CHAR, 0, 148, MPI_COMM_WORLD);
    MPI_Send(&num_of_steps, 1, MPI_INT, 0, 123, MPI_COMM_WORLD);
    //cout << "SLAVE OUT #" << rank << "\n"
}

int main(int argc,char **argv) {
    srand(time(0));
    int init;
    init = MPI_Init(&argc, &argv);
    if (init != MPI_SUCCESS){
        cout << "Fail\n";
        return 1;
    }
    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //cout << "Hi\n";
    table field;
    int n, m;
    int num_of_steps;
    int stop_bite = 0;
    bool justleave = false;
        while (!justleave){
            if (rank == 0){
                cout << "Hello!\n";
                if (cluster){
                    n = 1000;
                    m = 1000;
                    num_of_steps = 300;
                    field.assign(n, vector<char> (m,0));
                    generate_field(field);
                    double time_point1 = MPI_Wtime();
                    parallel_king(field, size - 1, num_of_steps);
                    int len_of_string = field[0].size();
                    int last = 0;
                    for (int proc = 1; proc < size; proc++){
                        int temp_size;
                        MPI_Recv(&temp_size, 1, MPI_INT, proc, 148, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        char temp_array[temp_size];
                        MPI_Recv(&temp_array, temp_size, MPI_CHAR, proc, 148, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        table temp_field;
                        array_to_matrix(temp_array, temp_size, temp_field, len_of_string);
                        for (int i = 0; i < temp_field.size(); i++){
                            field[last] = temp_field[i];
                            last++;
                        }
                    }
                    MPI_Recv(&num_of_steps, 1, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    double time_point2 = MPI_Wtime();
                    cout << setprecision(3) << time_point2 - time_point1 << "\n";
                    justleave = true;
                    int buffer = 0;
                    for (int proc = 1; proc < size; proc++)
                        MPI_Send(&buffer, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
                }
                else{
                    //cout << "Hello!\n";
                    string s;
                    cin >> s;
                    if (s == "START"){
                        string path = standart_path;
                        read_from_file(path, field);
                        n = field.size();
                        m = field[0].size();
                        cout << "The game has started\n";
                    }
                    else if (s == "RANDOM"){
                        cin >> n >> m;
                        field.assign(n, vector<char> (m,0));
                        generate_field(field);
                        cout << "The field has generated\n";
                    }
                    else if (s == "RUN"){
                        cin >> num_of_steps;
                        parallel_king(field, size - 1, num_of_steps);
                        cout << "The game is running\n";
                
                    }
                    else if (s == "STATUS"){
                        cout << "Field:\n";
                        write_field(field);
                    }
                    else if (s == "STOP"){
                        stop_bite = 1;
                        MPI_Send(&stop_bite, 1, MPI_INT, 1, 153, MPI_COMM_WORLD);
                        int len_of_string = field[0].size();
                        int last = 0;
                        for (int proc = 1; proc < size; proc++){
                            int temp_size;
                            MPI_Recv(&temp_size, 1, MPI_INT, proc, 148, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                            char temp_array[temp_size];
                            MPI_Recv(&temp_array, temp_size, MPI_CHAR, proc, 148, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        //cout << proc << "After stop size=" << temp_size << "\n";
                        //for (int i = 0; i < temp_size; i++)
                        //    cout << (int)temp_array[i];
                        //cout << "\n";
                            table temp_field;
                            array_to_matrix(temp_array, temp_size, temp_field, len_of_string);
                            for (int i = 0; i < temp_field.size(); i++){
                                field[last] = temp_field[i];
                                last++;
                            }
                        }
                        MPI_Recv(&num_of_steps, 1, MPI_INT, 1, 123, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        cout << "num_of_steps: " << num_of_steps << '\n';
                        cout << "Game stopped\n";
                    }
                    else if (s == "QUIT"){
                        justleave = true;
                        int buffer = 0;
                        for (int proc = 1; proc < size; proc++)
                            MPI_Send(&buffer, 1, MPI_INT, proc, 228, MPI_COMM_WORLD);
                    }
                    else cout << "String is not correct";
                }
            }
            else
                parallel_slave(rank);
        
    }
    //cout << "PROC #" << rank << " ended\n";
    MPI_Finalize();
    
    return 0;
}
