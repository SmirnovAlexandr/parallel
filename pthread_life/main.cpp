#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include <cmath>
#include <ctime>
#include <stdio.h>
#include <sys/time.h>
#include <iomanip>

using namespace std;

typedef vector<vector<char>> table;

const string standart_path = "/Users/Alexander/Documents/PROGA/1task/1task/test.csv";

table field1, field2;
int num_of_threads;
int maintable = 0;
int stop = 0;
int n,m;
int num_of_steps;
vector<pair<int, int>> parts;
vector<pthread_t> threads;

pthread_mutex_t mutex1;
pthread_cond_t conditional1;
int num_of_finished_threads, num_of_active_threads;

struct timeval start_time, end_time;

void mainstart(){
    parts.resize(num_of_threads);
    field1.assign(n, vector<char>(m,0));
    threads.resize(num_of_threads);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            field1[i][j] = rand() % 2;
    field2 = field1;
    
}

void read_from_file(string name_of_file){
    parts.resize(num_of_threads);
    threads.resize(num_of_threads);

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

void spread_parts(){
    int last = 0;
    for (int i = 0; i < num_of_threads; i++){
        pair<int, int> temp;
        temp.first = last;
        last += n / num_of_threads;
        if (n % num_of_threads > i) last++;
        temp.second = last - 1;
        parts[i] = temp;
    }
}

void initialization(){
    pthread_mutex_init(&mutex1, NULL);
    pthread_cond_init(&conditional1, NULL);
    num_of_finished_threads = 0;
    num_of_active_threads = num_of_threads;
    stop = 0;
}

void* fun(void* arg) {
    pair<int, int> area = *((pair<int, int>*)arg);
    for (int step = 0; step < num_of_steps; step++){
        for (int i = area.first; i <= area.second; i++){
            for (int j = 0; j < m; j++){
                check_neighbors(i,j);
            }
        }
        pthread_mutex_lock(&mutex1);
        num_of_active_threads--;
        if (num_of_active_threads > 0) {
            pthread_cond_wait(&conditional1, &mutex1);
        } else {
            num_of_active_threads = num_of_threads;
            maintable = (maintable + 1) % 2;
            if (stop) {
                num_of_steps = step;
            }
            pthread_cond_broadcast(&conditional1);
        }
        pthread_mutex_unlock(&mutex1);

    }
    return NULL;
}
void run_parallel(){
    initialization();
    spread_parts();
    for (int i = 0; i < num_of_threads; i++)
        pthread_create(&threads[i], NULL, fun, &parts[i]);
        

}

void completely_join() {
    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}


int main() {
    srand(time(0));
    bool justleave = false;
    cout << "Hello!\n";
    while (!justleave){
        string s;
        cin >> s;
        if (s == "START"){
            string path = standart_path;
            cin >> num_of_threads;
            read_from_file(path);
            cout << "The game has started\n";
        }
        else if (s == "RANDOM"){
            cin >> n >> m >> num_of_threads;
            mainstart();
            cout << "The field has generated\n";
        }
        else if (s == "RUN"){
            cin >> num_of_steps;
            run_parallel();
            cout << "The game is running\n";
            
        }
        else if (s == "STATUS"){
            cout << "Field:\n";
            write_field();
            cout << num_of_steps << "\n";
        }
        else if (s == "STOP"){
            stop = 1;
            for (int i = 0; i < num_of_threads; i++)
                pthread_join(threads[i], NULL);
            cout << "Number of operations: " << num_of_steps << "\n";
        }
        else if (s == "QUIT"){
            justleave = true;
            for (int i = 0; i < num_of_threads;i++)
                pthread_join(threads[i], NULL);
            
            pthread_mutex_destroy(&mutex1);
            pthread_cond_destroy(&conditional1);
        }
        else if (s == "TIME"){
            cin >> num_of_steps;
            int tempnum = num_of_threads;
            num_of_threads = 1;
            gettimeofday(&start_time, NULL);
            run_parallel();
            completely_join();
            gettimeofday(&end_time, NULL);
            int nptime = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
            cout << "Not parallel time: " << nptime << " microsec\n";
            num_of_threads = tempnum;
            gettimeofday(&start_time, NULL);
            run_parallel();
            completely_join();
            gettimeofday(&end_time, NULL);
            int ptime = (end_time.tv_sec - start_time.tv_sec) * 1000000 + (end_time.tv_usec - start_time.tv_usec);
            cout << "Parallel time: " << ptime << " microsec\n";
            cout << "Acceleration: " << setprecision(3) << (double)nptime/ptime << "\n";
            
        }
        else cout << "Unknown command\n";
    }
    return 0;
}



