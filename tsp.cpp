#include <bits/stdc++.h>
using namespace std;

using ll = long long;
using pii = pair<double,double>;

vector<pii> points;
int n;

void generate_input(int N, int seed = 42) 
{
    n = N;
    points.resize(n);

    mt19937 rng(seed);
    uniform_real_distribution<double> dist(0, 1000);

    for(int i = 0; i < n; i++) 
    {
        points[i] = {dist(rng), dist(rng)};
    }
}

double dist(int i, int j) 
{
    double dx = points[i].first - points[j].first;
    double dy = points[i].second - points[j].second;
    return sqrt(dx*dx + dy*dy);
}

double total_cost(const vector<int>& path) 
{
    double cost = 0;
    for(int i = 0; i < n - 1; i++)
        cost += dist(path[i], path[i+1]);
    cost += dist(path[n-1], path[0]);
    return cost;
}

// greedy
vector<int> greedy() 
{
    vector<bool> visited(n, false);
    vector<int> path;

    int cur = 0;
    path.push_back(cur);
    visited[cur] = true;

    for(int step = 1; step < n; step++) 
    {
        double best = 1e18;
        int next = -1;

        for(int j = 0; j < n; j++) 
        {
            if(!visited[j]) 
            {
                double d = dist(cur, j);
                if(d < best) 
                {
                    best = d;
                    next = j;
                }
            }
        }

        path.push_back(next);
        visited[next] = true;
        cur = next;
    }

    return path;
}

// 2opt 
void two_opt(vector<int>& path) 
{
    bool improved = true;

    while(improved) 
    {
        improved = false;

        for(int i = 1; i < n - 2; i++) 
        {
            for(int j = i + 1; j < n - 1; j++) 
            {

                double before =
                    dist(path[i-1], path[i]) +
                    dist(path[j], path[j+1]);

                double after =
                    dist(path[i-1], path[j]) +
                    dist(path[i], path[j+1]);

                if(after < before) 
                {
                    reverse(path.begin() + i, path.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
}

// multistart 
vector<int> multi_start(int iterations) 
{
    vector<int> best_path;
    double best_cost = 1e18;

    vector<int> base(n);
    iota(base.begin(), base.end(), 0);

    mt19937 rng(42);

    for(int it = 0; it < iterations; it++) 
    {
        shuffle(base.begin(), base.end(), rng);

        vector<int> path = base;
        two_opt(path);

        double cost = total_cost(path);

        if(cost < best_cost) 
        {
            best_cost = cost;
            best_path = path;
        }
    }

    return best_path;
}

// memory
size_t get_memory_usage() 
{
    return sizeof(points) + sizeof(double)*points.size()*2;
}

// run 
void run(string name, function<vector<int>()> solver) 
{
    auto start = chrono::high_resolution_clock::now();

    vector<int> path = solver();

    auto end = chrono::high_resolution_clock::now();

    double cost = total_cost(path);
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Algorithm: " << name << "\n";
    cout << "Cost: " << cost << "\n";
    cout << "Time(ms): " << duration.count() << "\n";
    cout << "Memory(bytes): " << get_memory_usage() << "\n";

    cout << "Path:\n";
    for(int x : path) cout << x << " ";
    cout << "\n\n";
}



int main(int argc, char* argv[]) 
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // ./a.out gen 500
    // ./a.out < input.txt

    if(argc >= 2 && string(argv[1]) == "gen")
    {
        int N = 500;
        if(argc >= 3) N = stoi(argv[2]);

        generate_input(N);

        cout << "Generated " << N << " points\n";
    } 
    else 
    {
        cin >> n;
        points.resize(n);
        for(int i = 0; i < n; i++) 
        {
            cin >> points[i].first >> points[i].second;
        }
    }

    run("Greedy", []() 
    {
        return greedy();
    });

    run("Greedy + 2-opt", []() 
    {
        auto p = greedy();
        two_opt(p);
        return p;
    });

    run("Multi-start (2-opt)", []() 
    {
        return multi_start(50);
    });

    return 0;
}
