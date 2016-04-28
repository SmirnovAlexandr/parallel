#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <cmath>
#include <ctime>
#include <stdio.h>
#include <sys/time.h>
#include <iomanip>
#include <cstdlib>
#include <omp.h>

using namespace std;

typedef vector<vector<char> > table;

const string standart_path = "/home/parallels/MPI/test.csv";

table field1, field2;
int num_of_threads;
int maintable = 0;
int stop = 0;
int time_res = 0;
int n,m;
int num_of_steps;

struct timeval start_time, end_time;

void generate_field(){
    field1.assign(n, vector<char>(m,0));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            field1[i][j] = rand() % 2;
    field2 = field1;
}

void read_from_file(){
    ifstream file;
    file.open("/home/parallels/MPI/test.csv", std::ifstream::in);
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
        field1.push_back(temp_vector);
    }
    field2 = field1;
    n = field1.size();
    m = field1[0].size();
}

void write_field(){
    table *ptr;
    if (maintable == 0) ptr = &field1;
    else ptr = &field2;
    
    for (int i = 0; i < n; i++){
        for (int j = 0; j < m; j++){
            if ((*ptr)[i][j] == 0) cout << ' ';
            else cout << '1';
        }
        cout << "\n";
    }
}

void check_neighbors(int x, int y){
    table *ptr1, *ptr2;
    if (maintable == 0){
        ptr1 = &field1;
        ptr2 = &field2;
    }
    else{
        ptr1 = &field2;
        ptr2 = &field1;
    }
    int num = 0;
    for (int i = - 1; i <= 1; i++)
        for (int j = -1; j<= 1; j++){
            int nx = x + i;
            int ny = y + j;
            if (nx >= n) nx = 0;
            else if (nx < 0) nx = n - 1;
            if (ny >= m) ny = 0;
            else if (ny < 0) ny = m - 1;
            if ((*ptr1)[nx][ny] == 1 && (i != 0 || j != 0))
                num++;
        }
    if (num == 3)
        (*ptr2)[x][y] = 1;
    else if (num == 2 && (*ptr1)[x][y] == 1) (*ptr2)[x][y] = 1;
    else (*ptr2)[x][y] = 0;
    
}

int main(){
    srand(time(0));
    bool justleave = false;
    cout << "Hello!\n";
    int psignal = 0;
    #pragma omp parallel num_threads(2)
    {        
        //Main section
        while (!justleave){
            #pragma omp master
            {        
                string s;
                cin >> s;
                if (s == "START"){
                    cin >> num_of_threads;
                    read_from_file();
                    cout << "The game has started\n";
                }
                else if (s == "RANDOM"){
                    cin >> n >> m >> num_of_threads;
                    generate_field();
                    cout << "The field has generated\n";
                }
                else if (s == "RUN"){
                    cin >> num_of_steps;
                    psignal = 1;
                    cout << "The game is running\n"; 
                }
                else if (s == "STATUS"){
                    cout << "Field:\n";
                    write_field();
                    cout << num_of_steps << "\n";
                }
                else if (s == "STOP"){
                    stop = 1;
                }
                else if (s == "QUIT"){
                    justleave = true;
                }
                else if (s == "TEST"){
                    cout << "Enter 'num_of_threads' and 'num_of_steps'\n";
                    cin >> num_of_threads >> num_of_steps;
                    n = m = 1000;
                    generate_field();
                    gettimeofday(&start_time, NULL);
                    psignal = 1;
                    time_res = 1;
                    justleave = true;
                }
                else cout << "Unknown command\n";
            }
            #pragma omp barrier
            int id = omp_get_thread_num();
            if (id == 1 & psignal == 1){
                psignal = 0;
                #pragma omp parallel num_threads(num_of_threads)
                {
                    for (int steps = 0; steps < num_of_steps; steps++){
                        #pragma omp for schedule(static)
                            for (int i = 0; i < n; i++){
                                for (int j = 0; j < m;j++){
                                    check_neighbors(i,j);
                                }
                            }
                        #pragma omp single
                        {
                            if (stop == 1){ 
                                num_of_steps = steps;
                                stop = 2;
                                cout << "Game has stoped. Number of operations: " << num_of_steps << '\n';
                            }
                            maintable = (maintable + 1) % 2;
                        }
                    }
                    
                }
                if (stop == 0){
                    cout << "Game has stoped. Number of operations: " << num_of_steps << '\n';
                }
            }

        }
        #pragma omp barrier
        #pragma omp master
        {
            if (time_res == 1){
                gettimeofday(&end_time, NULL);
                int nptime = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
                cout << "Time with num_of_threads = " << num_of_threads << " and num_of_steps = " << num_of_steps << " is " << nptime << " microsec\n";
                
            }
        }

    }



    return 0;
}