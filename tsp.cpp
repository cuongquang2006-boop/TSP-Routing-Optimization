#include <bits/stdc++.h>
#include <iostream>
using namespace std;

using pii = pair<double,double>;

vector<pii> points;
int n;

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

mt19937& get_rng() 
{
    static mt19937 rng(random_device{}());
    return rng;
}

vector<int> simulated_annealing()
{

    vector<int> cur(n);
    for(int i = 0; i < n; i++) cur[i] = i;

    //ramdom 
    shuffle(cur.begin(), cur.end(), get_rng());

    // gán đường tốt nhất hiện tại là đường đã random
    vector<int> best = cur;
    double best_cost = total_cost(cur); //lấy tổng trị giá đường hiện tại

    if(n <= 1) return best; // hoặc skip

    double T = 1000;
    double alpha = 0.995;

    while(T > 1e-3)
    {
        // gán vào đường tiếp theo 
        vector<int> next = cur;

        // lấy ngẫu nhiên vị trí trong tour
        int i = get_rng()() % n; 
        int j = get_rng()() % n;
        while(i == j) j = get_rng()() % n;
        swap(next[i], next[j]);

        double cur_cost = total_cost(cur);
        double next_cost = total_cost(next);

        double delta = next_cost - cur_cost;

        double prob = (double)get_rng()() / get_rng().max();

        if(delta < 0 || exp(-delta / T) > prob)
        {
            cur = next;
            if(next_cost < best_cost)
            {
                best_cost = next_cost;
                best = next;
            }
        }

        T *= alpha;
    }

    return best;
}

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

vector<int> nearest_insertion() 
{
    vector<bool> inTour(n, false);
    vector<int> tour;

    tour = {0, 1};
    inTour[0] = inTour[1] = true;

    while(tour.size() < n) 
    {
        int best_node = -1;
        double best_dist = 1e18;

        for(int i = 0; i < n; i++) if(!inTour[i]) 
        {
            for(int j : tour) 
            {
                double d = dist(i, j);
                if(d < best_dist)
                {
                    best_dist = d;
                    best_node = i;
                }
            }
        }

        int best_pos = 0;
        double best_increase = 1e18;

        for(int i = 0; i < (int)tour.size(); i++) 
        {
            int u = tour[i];
            int v = tour[(i+1)%tour.size()];
            
            double inc = dist(u, best_node) + dist(best_node, v) - dist(u, v);
            if(inc < best_increase) 
            {
                best_increase = inc;
                best_pos = i + 1;
            }
        }
        
        tour.insert(tour.begin() + best_pos, best_node);
        inTour[best_node] = true;
    }

    return tour;
}

double best_cost;
vector<int> best_path;

double bound_estimate(const vector<bool>& visited) 
{
    double estimate = 0;
    for(int i = 0; i < n; i++) if(!visited[i]) 
    {
        double mn = 1e18; 
        for(int j = 0; j < n; j++) if(i != j)
        { 
            mn = min(mn, dist(i,j));
        }
        estimate += mn; 
    }
    return estimate;
}

void dfs(vector<int>& path, vector<bool>& visited, double cost) 
{
    if(path.size() == n) 
    {
        cost += dist(path.back(), path[0]);
        if(cost < best_cost) 
        {
            best_cost = cost;
            best_path = path;
        }
        return;
    }

    double b = cost + bound_estimate(visited);
    if(b >= best_cost) return;

    for(int i = 0; i < n; i++) if(!visited[i]) 
    {
        visited[i] = true;
        path.push_back(i);

        double new_cost = cost;
        if(path.size() > 1)
            new_cost += dist(path[path.size()-2], i);

        dfs(path, visited, new_cost);

        visited[i] = false;
        path.pop_back();
    }
}

vector<int> branch_and_bound() 
{
    best_cost = 1e18;
    best_path.clear();

    vector<int> path;
    vector<bool> visited(n, false);

    path.push_back(0);
    visited[0] = true;

    dfs(path, visited, 0);
    return best_path;
} 

void run(string name, function<vector<int>()> solver) 
{
    cout << "============================\n";
    cout << "Algorithm: " << name << "\n";

    const int ITER = 1000;
    vector<int> warm = solver();
    auto start = chrono::high_resolution_clock::now();

    vector<int> path;
    for(int i = 0; i < ITER; i++) 
    {
        path = solver();
    }
    auto end = chrono::high_resolution_clock::now();
    double cost = total_cost(path);
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    double avg_time = (double)duration.count() / ITER;

    cout << fixed << setprecision(2);
    cout << "Cost: " << cost << "\n";
    cout << "Avg Time (us): " << avg_time << "\n";
    cout << "Path: ";
    for(int x : path) cout << x << " ";
    cout << "\n\n";
}

void run_bb()
{
    cout << "============================\n";
    cout << "Algorithm: Branch & Bound\n";

    vector<int> warm = branch_and_bound();
    auto start = chrono::high_resolution_clock::now();
    vector<int> path = branch_and_bound();
    auto end = chrono::high_resolution_clock::now();
    double cost = total_cost(path);
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << fixed << setprecision(2);
    cout << "Cost: " << cost << "\n";
    cout << "Time (us): " << duration.count() << "\n";
    cout << "Path: ";
    for(int x : path) cout << x << " ";
    cout << "\n\n";
}

int main() 
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    freopen("input500.txt", "r", stdin);

    cin >> n;
    points.resize(n);
    for(int i = 0; i < n; i++)
        cin >> points[i].first >> points[i].second;

    run("Simulated Annealing", simulated_annealing);
    run("Greedy", greedy);
    run("Nearest Insertion", nearest_insertion);

    if(n <= 12)
        run_bb();

    return 0;
}