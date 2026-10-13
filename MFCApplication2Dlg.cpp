#include "pch.h"
#include "framework.h"
#include "MFCApplication2.h"
#include "MFCApplication2Dlg.h"
#include "afxdialogex.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <random>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx {
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CMFCApplication2Dlg::CMFCApplication2Dlg(CWnd* pParent)
    : CDialogEx(IDD_MFCAPPLICATION2_DIALOG, pParent)
    , m_nRows(10), m_nCols(10)
    , m_startPoint(0, 0), m_endPoint(9, 9)
    , m_agvIndex(0), m_isRunning(false)
    , m_speed(200), m_turnTime(100)
    , m_cellSize(40) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMFCApplication2Dlg::~CMFCApplication2Dlg() {
    // STL vector handles memory automatically
}

void CMFCApplication2Dlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_ROWS, m_nRows);
    DDX_Text(pDX, IDC_EDIT_COLS, m_nCols);
    DDX_Text(pDX, IDC_EDIT_SPEED, m_speed);
    DDX_Text(pDX, IDC_EDIT_TURNTIME, m_turnTime);
    DDV_MinMaxInt(pDX, m_nRows, 5, 50);
    DDV_MinMaxInt(pDX, m_nCols, 5, 50);
    DDV_MinMaxInt(pDX, m_speed, 50, 1000);
    DDV_MinMaxInt(pDX, m_turnTime, 0, 500);
}

BEGIN_MESSAGE_MAP(CMFCApplication2Dlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BUTTON_GENERATE, &CMFCApplication2Dlg::OnBnClickedButtonGenerate)
    ON_BN_CLICKED(IDC_BUTTON_START, &CMFCApplication2Dlg::OnBnClickedButtonStart)
    ON_BN_CLICKED(IDC_BUTTON_STOP, &CMFCApplication2Dlg::OnBnClickedButtonStop)
END_MESSAGE_MAP()

// CMFCApplication2Dlg message handlers

BOOL CMFCApplication2Dlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu) {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    
    InitMaze(m_nRows, m_nCols);
    GenerateMaze();
    Dijkstra(m_startPoint, m_endPoint, m_path);
    if (!CalculateFastestPath(m_startPoint, m_endPoint, m_fastestPath))
        m_fastestPath = m_path;
    m_agvPosition = m_startPoint;
    CalculateDrawRect();
    
    return TRUE;
}

void CMFCApplication2Dlg::OnSysCommand(UINT nID, LPARAM lParam) {
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CMFCApplication2Dlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CPaintDC dc(this);
        CRect rect;
        GetClientRect(&rect);
        dc.FillSolidRect(rect, RGB(255, 255, 255));
        
        DrawMaze(&dc);
        DrawPath(&dc, m_path, RGB(0, 0, 255));
        DrawPath(&dc, m_fastestPath, RGB(0, 255, 0));
        DrawAGV(&dc);
    }
}

HCURSOR CMFCApplication2Dlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

// Initialize maze with enhanced memory management
void CMFCApplication2Dlg::InitMaze(int rows, int cols)
{
    // Reset paths and AGV state
    m_path.clear();
    m_fastestPath.clear();
    m_agvPosition = Point(0, 0);
    m_agvIndex = 0;
    m_isRunning = false;
    KillTimer(1); // Ensure timer is stopped
    
    // Reset drawing rect to ensure complete redraw
    m_drawRect.SetRect(0, 0, 0, 0);

    // Validate rows and columns to ensure they are within reasonable limits
    int newRows = max(rows, 1);
    newRows = min(newRows, 100); // Limit maximum rows to 100 to prevent excessive memory usage
    int newCols = max(cols, 1);
    newCols = min(newCols, 100); // Limit maximum columns to 100 to prevent excessive memory usage

    // Check if we need to show a warning about total cells limitation
    bool showWarning = false;
    CString warningMessage;

    // Use vector for safe memory management
    try
    {
        // Check if we can safely allocate the maze (prevent excessive memory usage)
        const size_t maxTotalCells = 10000; // Limit total cells to prevent out-of-memory
        if (static_cast<size_t>(rows) * static_cast<size_t>(cols) > maxTotalCells)
        {
            // Reduce size to safe level
            int originalRows = rows;
            int originalCols = cols;
            
            newRows = static_cast<int>(sqrt(maxTotalCells));
            newCols = static_cast<int>(sqrt(maxTotalCells));
            
            showWarning = true;
            warningMessage.Format(L"Warning: Maze too large, automatically adjusted size.");
        }
        // The following else if condition is redundant and will never be true
        // because newRows and newCols are already limited to 100 each
        // else if (static_cast<size_t>(newRows) * static_cast<size_t>(newCols) > maxTotalCells)
        // {
        //     // Reduce size to safe level
        //     int originalRows = newRows;
        //     int originalCols = newCols;
        //     
        //     newRows = static_cast<int>(sqrt(maxTotalCells));
        //     newCols = static_cast<int>(sqrt(maxTotalCells));
        //     
        //     showWarning = true;
        //     warningMessage.Format(L"Warning: Maze too large, automatically adjusted size.");
        // }

        // Initialize maze using vector for safe memory management
        m_maze.resize(newRows);
        for (int i = 0; i < newRows; i++)
        {
            m_maze[i].resize(newCols, EMPTY);
        }

        m_nRows = newRows;
        m_nCols = newCols;
        
        // Randomize start and end points
        InitializeRandomStartEnd();
    }
    catch (std::bad_alloc&)
    {
        // Handle memory allocation failure with progressive fallback strategy
        // Vector will automatically free memory on exception
        
        // Progressive fallback to smaller sizes
        int fallbackSizes[] = { 50, 25, 15, 10, 5 };
        bool fallbackSuccess = false;
        
        for (int fallbackSize : fallbackSizes)
        {
            try
            {
                // Reset to safe default values
                m_nRows = fallbackSize;
                m_nCols = fallbackSize;
                
                // Randomize start and end points
                InitializeRandomStartEnd();
                
                // Allocate maze using vector
                m_maze.resize(m_nRows);
                for (int i = 0; i < m_nRows; i++)
                {
                    m_maze[i].resize(m_nCols, EMPTY);
                }
                
                fallbackSuccess = true;
                break;
            }
            catch (std::bad_alloc&)
            {
                // Vector will automatically free memory on exception
                m_maze.clear();
            }
        }
        
        // If all fallback attempts fail, set to minimal 1x1 maze as last resort
        if (!fallbackSuccess)
        {
            try
            {
                // Set minimal maze dimensions
                m_nRows = 1;
                m_nCols = 1;
                m_startPoint = Point(0, 0);
                m_endPoint = Point(0, 0);
                
                // Create minimal 1x1 maze
                m_maze.resize(1);
                m_maze[0].resize(1, EMPTY);
            }
            catch (std::bad_alloc&)
            {
                // Even 1x1 maze failed - set to empty state as last resort
                m_maze.clear();
                m_nRows = 0;
                m_nCols = 0;
            }
        }
    }

    // Show warning message if maze size was adjusted
    if (showWarning && !m_maze.empty())
    {
        MessageBox(warningMessage, L"Maze Size Adjusted", MB_ICONWARNING | MB_OK);
    }
}

// Union-Find data structure for Kruskal's algorithm
class UnionFind {
private:
    std::vector<int> parent;
    std::vector<int> rank;

public:
    UnionFind(int size) {
        parent.resize(size);
        rank.resize(size, 0);
        for (int i = 0; i < size; i++) {
            parent[i] = i;
        }
    }

    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // Path compression
        }
        return parent[x];
    }

    bool unite(int x, int y) {
        x = find(x);
        y = find(y);

        if (x == y) {
            return false; // Already in the same set
        }

        // Union by rank
        if (rank[x] < rank[y]) {
            parent[x] = y;
        } else {
            parent[y] = x;
            if (rank[x] == rank[y]) {
                rank[x]++;
            }
        }
        return true;
    }
};

// Initialize start and end points randomly with sufficient distance
void CMFCApplication2Dlg::InitializeRandomStartEnd()
{
    // Guard against mazes too small to place start/end in the interior.
    // For side length < 5, uniform_int_distribution ranges like (1, n-2)
    // or the quadrant fallback (mid+1, n-2) become invalid (a > b), which
    // is undefined behaviour. Fall back to opposite corners instead.
    if (m_nRows < 5 || m_nCols < 5) {
        m_startPoint = Point(0, 0);
        m_endPoint = Point(m_nRows > 0 ? m_nRows - 1 : 0, m_nCols > 0 ? m_nCols - 1 : 0);
        return;
    }

    // Initialize random number generator
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Create random distributions for coordinates
    std::uniform_int_distribution<> rowDist(1, m_nRows - 2);
    std::uniform_int_distribution<> colDist(1, m_nCols - 2);
    
    // Calculate minimum distance threshold (at least 30% of maze diagonal)
    double mazeDiagonal = std::sqrt(static_cast<double>(m_nRows * m_nRows + m_nCols * m_nCols));
    double minDistance = (std::max)(5.0, mazeDiagonal * 0.3); // At least 5 cells or 30% of diagonal
    
    // Randomize start point
    m_startPoint.x = rowDist(gen);
    m_startPoint.y = colDist(gen);
    
    // Randomize end point ensuring sufficient distance from start
    int attempts = 0;
    const int maxAttempts = 100;
    
    do {
        m_endPoint.x = rowDist(gen);
        m_endPoint.y = colDist(gen);
        attempts++;
        
        // Calculate Manhattan distance between start and end
        int manhattanDistance = std::abs(m_endPoint.x - m_startPoint.x) + std::abs(m_endPoint.y - m_startPoint.y);
        
        // If distance is sufficient or we've tried too many times, break
        if (manhattanDistance >= minDistance || attempts >= maxAttempts) {
            break;
        }
    } while (true);
    
    // If we couldn't find a suitable end point after max attempts,
    // try a different approach: place end point in opposite quadrant
    if (attempts >= maxAttempts) {
        // Divide maze into quadrants and place points in opposite quadrants
        int midRow = m_nRows / 2;
        int midCol = m_nCols / 2;
        
        // Start in top-left quadrant
        if (m_startPoint.x <= midRow && m_startPoint.y <= midCol) {
            // End in bottom-right quadrant
            std::uniform_int_distribution<> endRowDist(midRow + 1, m_nRows - 2);
            std::uniform_int_distribution<> endColDist(midCol + 1, m_nCols - 2);
            m_endPoint.x = endRowDist(gen);
            m_endPoint.y = endColDist(gen);
        }
        else if (m_startPoint.x <= midRow && m_startPoint.y > midCol) {
            // End in bottom-left quadrant
            std::uniform_int_distribution<> endRowDist(midRow + 1, m_nRows - 2);
            std::uniform_int_distribution<> endColDist(1, midCol);
            m_endPoint.x = endRowDist(gen);
            m_endPoint.y = endColDist(gen);
        }
        else if (m_startPoint.x > midRow && m_startPoint.y <= midCol) {
            // End in top-right quadrant
            std::uniform_int_distribution<> endRowDist(1, midRow);
            std::uniform_int_distribution<> endColDist(midCol + 1, m_nCols - 2);
            m_endPoint.x = endRowDist(gen);
            m_endPoint.y = endColDist(gen);
        }
        else {
            // Start in bottom-right quadrant, end in top-left quadrant
            std::uniform_int_distribution<> endRowDist(1, midRow);
            std::uniform_int_distribution<> endColDist(1, midCol);
            m_endPoint.x = endRowDist(gen);
            m_endPoint.y = endColDist(gen);
        }
    }
}

void CMFCApplication2Dlg::GenerateMaze() {
    if (m_maze.empty() || m_nRows <= 0 || m_nCols <= 0) return;

    for (int i = 0; i < m_nRows; i++)
        for (int j = 0; j < m_nCols; j++)
            m_maze[i][j] = OBSTACLE;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    constexpr int dx[] = { -1, 1, 0, 0 };
    constexpr int dy[] = { 0, 0, -1, 1 };

    std::vector<std::vector<bool>> visited(m_nRows, std::vector<bool>(m_nCols, false));
    std::vector<Point> stack;
    
    stack.push_back(m_startPoint);
    visited[m_startPoint.x][m_startPoint.y] = true;
    m_maze[m_startPoint.x][m_startPoint.y] = EMPTY;

    while (!stack.empty()) {
        Point current = stack.back();
        stack.pop_back();

        std::vector<int> validDirs;
        for (int d = 0; d < 4; d++) {
            int ni = current.x + dx[d] * 2;
            int nj = current.y + dy[d] * 2;
            if (ni >= 1 && ni < m_nRows - 1 && nj >= 1 && nj < m_nCols - 1 && !visited[ni][nj])
                validDirs.push_back(d);
        }

        if (!validDirs.empty()) {
            stack.push_back(current);
            int d = validDirs[std::uniform_int_distribution<>(0, validDirs.size() - 1)(gen)];
            
            int ni = current.x + dx[d] * 2;
            int nj = current.y + dy[d] * 2;
            int mi = current.x + dx[d];
            int mj = current.y + dy[d];
            
            visited[ni][nj] = true;
            m_maze[mi][mj] = EMPTY;
            m_maze[ni][nj] = EMPTY;
            stack.push_back(Point(ni, nj));
        }
    }

    std::uniform_real_distribution<> probDist(0.0, 1.0);
    for (int i = 0; i < m_nRows; i++) {
        for (int j = 0; j < m_nCols; j++) {
            if (m_maze[i][j] == OBSTACLE) {
                int emptyNeighbors = 0;
                for (int d = 0; d < 4; d++) {
                    int ni = i + dx[d];
                    int nj = j + dy[d];
                    if (ni >= 0 && ni < m_nRows && nj >= 0 && nj < m_nCols && m_maze[ni][nj] == EMPTY)
                        emptyNeighbors++;
                }
                
                if (i > 2 && i < m_nRows - 3 && j > 2 && j < m_nCols - 3) {
                    if (emptyNeighbors >= 2 && probDist(gen) < 0.4)
                        m_maze[i][j] = EMPTY;
                } else if (emptyNeighbors >= 3 && probDist(gen) < 0.25) {
                    m_maze[i][j] = EMPTY;
                }
            }
        }
    }

    for (int i = 0; i < m_nRows; i++) {
        for (int j = 0; j < m_nCols; j++) {
            if ((i == m_startPoint.x && j == m_startPoint.y) || (i == m_endPoint.x && j == m_endPoint.y))
                continue;
            
            if (m_maze[i][j] == EMPTY) {
                int emptyNeighbors = 0;
                for (int d = 0; d < 4; d++) {
                    int ni = i + dx[d];
                    int nj = j + dy[d];
                    if (ni >= 0 && ni < m_nRows && nj >= 0 && nj < m_nCols && m_maze[ni][nj] == EMPTY)
                        emptyNeighbors++;
                }
                
                if ((i == 0 || i == m_nRows - 1 || j == 0 || j == m_nCols - 1) && emptyNeighbors >= 2 && probDist(gen) < 0.35) {
                    m_maze[i][j] = OBSTACLE;
                } else if ((j == 1 || j == m_nCols - 2 || i == 1 || i == m_nRows - 2) && emptyNeighbors >= 3 && probDist(gen) < 0.25) {
                    m_maze[i][j] = OBSTACLE;
                } else if (emptyNeighbors >= 4 && probDist(gen) < 0.15) {
                    m_maze[i][j] = OBSTACLE;
                }
            }
        }
    }

    m_maze[m_startPoint.x][m_startPoint.y] = EMPTY;
    m_maze[m_endPoint.x][m_endPoint.y] = EMPTY;

    if (m_startPoint != m_endPoint) {
        std::vector<std::vector<bool>> visited2(m_nRows, std::vector<bool>(m_nCols, false));
        std::queue<Point> q;
        q.push(m_startPoint);
        visited2[m_startPoint.x][m_startPoint.y] = true;
        bool foundPath = false;

        while (!q.empty() && !foundPath) {
            Point current = q.front();
            q.pop();

            if (current == m_endPoint) {
                foundPath = true;
                break;
            }

            for (int d = 0; d < 4; d++) {
                int nx = current.x + dx[d];
                int ny = current.y + dy[d];

                if (nx >= 0 && nx < m_nRows && ny >= 0 && ny < m_nCols && !visited2[nx][ny] && m_maze[nx][ny] == EMPTY) {
                    visited2[nx][ny] = true;
                    q.push(Point(nx, ny));
                }
            }
        }

        if (!foundPath) {
            Point current = m_endPoint;
            while (!(current.x == m_startPoint.x && current.y == m_startPoint.y)) {
                m_maze[current.x][current.y] = EMPTY;
                
                if (current.x > m_startPoint.x) current.x--;
                else if (current.x < m_startPoint.x) current.x++;
                else if (current.y > m_startPoint.y) current.y--;
                else if (current.y < m_startPoint.y) current.y++;
            }
            m_maze[m_startPoint.x][m_startPoint.y] = EMPTY;
        }
    }

    if (!m_maze.empty() && m_startPoint.x >= 0 && m_startPoint.x < m_nRows && m_startPoint.y >= 0 && m_startPoint.y < m_nCols)
        m_maze[m_startPoint.x][m_startPoint.y] = START;
    
    if (!m_maze.empty() && m_endPoint.x >= 0 && m_endPoint.x < m_nRows && m_endPoint.y >= 0 && m_endPoint.y < m_nCols)
        m_maze[m_endPoint.x][m_endPoint.y] = END;
}

// Check if position is valid
bool CMFCApplication2Dlg::IsValid(int x, int y)
{
    return !m_maze.empty() && x >= 0 && x < m_nRows && y >= 0 && y < m_nCols && m_maze[x][y] != OBSTACLE;
}

// Dijkstra algorithm for shortest path (distance-based)
bool CMFCApplication2Dlg::Dijkstra(Point start, Point end, std::vector<Point>& path)
{
    path.clear();

    // Validate maze and dimensions
    if (m_maze.empty() || m_nRows <= 0 || m_nCols <= 0)
        return false;

    // Validate start and end points
    if (start.x < 0 || start.x >= m_nRows || start.y < 0 || start.y >= m_nCols ||
        end.x < 0 || end.x >= m_nRows || end.y < 0 || end.y >= m_nCols)
    {
        return false;
    }

    // Directions: up, down, left, right
    constexpr int dx[] = { -1, 1, 0, 0 };
    constexpr int dy[] = { 0, 0, -1, 1 };

    // Initialize distance and parent arrays with boundary checks
    std::vector<std::vector<int>> distance;
    std::vector<std::vector<Point>> parent;
    std::vector<std::vector<bool>> visited;
    
    try
    {
        distance = std::vector<std::vector<int>>(m_nRows, std::vector<int>(m_nCols, INT_MAX));
        parent = std::vector<std::vector<Point>>(m_nRows, std::vector<Point>(m_nCols, Point(-1, -1)));
        visited = std::vector<std::vector<bool>>(m_nRows, std::vector<bool>(m_nCols, false));
    }
    catch (std::bad_alloc&)
    {
        return false;
    }

    // Priority queue for Dijkstra's algorithm
    std::priority_queue<DijkstraNode> pq;

    // Start node with boundary check
    if (start.x >= 0 && start.x < m_nRows && start.y >= 0 && start.y < m_nCols)
    {
        distance[start.x][start.y] = 0;
        pq.push(DijkstraNode(start, 0, Point(-1, -1)));
    }

    while (!pq.empty())
    {
        DijkstraNode current = pq.top();
        pq.pop();

        Point p = current.point;
        
        // Validate current point
        if (p.x < 0 || p.x >= m_nRows || p.y < 0 || p.y >= m_nCols)
            continue;
            
        if (visited[p.x][p.y]) continue;
        visited[p.x][p.y] = true;

        // Found the end point
        if (p == end)
        {
            // Reconstruct path with strict boundary checks
            Point curr = end;
            int safetyCounter = 0;
            const int maxSafetyIterations = m_nRows * m_nCols * 2;
            
            while (curr.x != -1 && curr.y != -1 && safetyCounter < maxSafetyIterations)
            {
                safetyCounter++;
                
                // Ensure we're accessing valid indices
                if (curr.x >= 0 && curr.x < m_nRows && curr.y >= 0 && curr.y < m_nCols &&
                    !parent.empty() && curr.x < parent.size() && !parent[curr.x].empty() && curr.y < parent[curr.x].size())
                {
                    path.push_back(curr);
                    Point next = parent[curr.x][curr.y];
                    curr = next;
                }
                else
                {
                    // Invalid indices, break to prevent crash
                    break;
                }
            }
            
            if (!path.empty())
            {
                std::reverse(path.begin(), path.end());
            }
            return !path.empty();
        }

        // Explore neighbors with boundary checks
        for (int i = 0; i < 4; i++)
        {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];

            if (nx >= 0 && nx < m_nRows && ny >= 0 && ny < m_nCols && IsValid(nx, ny) && !visited[nx][ny])
            {
                if (distance[p.x][p.y] != INT_MAX)
                {
                    int newDistance = distance[p.x][p.y] + 1;
                    if (newDistance < distance[nx][ny])
                    {
                        distance[nx][ny] = newDistance;
                        parent[nx][ny] = p;
                        pq.push(DijkstraNode(Point(nx, ny), newDistance, p));
                    }
                }
            }
        }
    }

    return false; // No path found
}

// Calculate fastest path considering turn time
bool CMFCApplication2Dlg::CalculateFastestPath(Point start, Point end, std::vector<Point>& path)
{
    path.clear();

    // Validate maze and dimensions
    if (m_maze.empty() || m_nRows <= 0 || m_nCols <= 0) return false;

    // Validate start and end points
    if (start.x < 0 || start.x >= m_nRows || start.y < 0 || start.y >= m_nCols ||
        end.x < 0 || end.x >= m_nRows || end.y < 0 || end.y >= m_nCols) {
        return false;
    }

    // Directions: up, down, left, right
    int dx[] = { -1, 1, 0, 0 };
    int dy[] = { 0, 0, -1, 1 };

    // Struct to hold node with direction information
    struct DirectionalNode {
        Point point;
        int distance;
        int direction;
        DirectionalNode(Point p, int d, int dir) : point(p), distance(d), direction(dir) {}
        bool operator<(const DirectionalNode& other) const {
            return distance > other.distance; // Min-heap
        }
    };

    // Priority queue for Dijkstra's algorithm with time cost
    std::priority_queue<DirectionalNode> pq;
    
    // Initialize distance and parent arrays with optimized memory allocation
    // Use a single vector with calculated indices instead of 3D vectors to reduce memory overhead
    const int TOTAL_CELLS = m_nRows * m_nCols;
    const int TOTAL_ELEMENTS = TOTAL_CELLS * 4;
    
    // Check if the total elements exceed a safe limit
    const int SAFE_MAX_ELEMENTS = 100000; // 100k elements should be safe
    if (TOTAL_ELEMENTS > SAFE_MAX_ELEMENTS) {
        // For large mazes, use a simpler 2D array without direction consideration
        // This will be faster and use less memory
        return false; // Indicate we need to use a simpler approach
    }
    
    std::vector<int> distance_flat;
    std::vector<std::pair<Point, int>> parent_flat;
    
    try {
        // Preallocate flat arrays
        distance_flat.resize(TOTAL_ELEMENTS, INT_MAX);
        parent_flat.resize(TOTAL_ELEMENTS, std::make_pair(Point(-1, -1), -1));
    } catch (std::bad_alloc&) {
        return false; // Memory allocation failed
    }
    
    // Helper function to calculate flat index with safety checks
    auto getIndex = [this](int x, int y, int dir) -> int {
        // Ensure all indices are within valid ranges
        if (x < 0 || x >= m_nRows || y < 0 || y >= m_nCols || dir < 0 || dir >= 4) {
            return -1; // Return invalid index if any parameter is out of range
        }
        return ((x * m_nCols) + y) * 4 + dir;
    };

    // Start with all directions at start point
    for (int dir = 0; dir < 4; dir++)
    {
        int idx = getIndex(start.x, start.y, dir);
        if (idx >= 0 && idx < distance_flat.size())
        {
            distance_flat[idx] = 0;
            pq.push(DirectionalNode(start, 0, dir));
        }
    }

    // Track the minimum distance to the end point and its direction
    int minEndDistance = INT_MAX;
    int endDir = -1;

    while (!pq.empty())
    {
        DirectionalNode current = pq.top();
        pq.pop();

        Point p = current.point;
        int dir = current.direction;
        
        // Calculate index for current node and direction
        int currentIdx = getIndex(p.x, p.y, dir);
        
        // Skip if index is invalid
        if (currentIdx < 0 || currentIdx >= distance_flat.size()) continue;
        
        // Skip if this is not the shortest path to this node in this direction
        if (distance_flat[currentIdx] < current.distance) continue;
        
        // Check if we've reached the end with a better distance
        if (p == end && current.distance < minEndDistance)
        {
            minEndDistance = current.distance;
            endDir = dir;
        }
        
        // Explore neighbors
        for (int newDir = 0; newDir < 4; newDir++)
        {
            int nx = p.x + dx[newDir];
            int ny = p.y + dy[newDir];

            if (IsValid(nx, ny))
            {
                int timeCost = m_speed;
                if (dir != newDir && current.distance != 0) // Not first move and direction changed
                {
                    timeCost += m_turnTime;
                }

                int newDistance = current.distance + timeCost;
                int neighborIdx = getIndex(nx, ny, newDir);
                
                // Ensure valid index before accessing arrays
                if (neighborIdx >= 0 && neighborIdx < distance_flat.size())
                {
                    if (newDistance < distance_flat[neighborIdx])
                    {
                        distance_flat[neighborIdx] = newDistance;
                        parent_flat[neighborIdx] = std::make_pair(p, dir);
                        pq.push(DirectionalNode(Point(nx, ny), newDistance, newDir));
                    }
                }
            }
        }
    }

    // If no path found
    if (endDir == -1) return false;
    
    // Reconstruct path
    Point curr = end;
    int dir = endDir;
    int safetyCounter = 0;
    const int maxSafetyIterations = m_nRows * m_nCols * 4; // Increased safety limit due to direction dimension
    
    while (curr.x != -1 && curr.y != -1 && safetyCounter < maxSafetyIterations)
    {
        safetyCounter++;
        // Ensure we're accessing valid indices
        if (curr.x >= 0 && curr.x < m_nRows && curr.y >= 0 && curr.y < m_nCols && dir >= 0 && dir < 4)
        {
            path.push_back(curr);
            int parentIdx = getIndex(curr.x, curr.y, dir);
            
            // Ensure valid index before accessing parent_flat
            if (parentIdx >= 0 && parentIdx < parent_flat.size())
            {
                auto parentInfo = parent_flat[parentIdx];
                Point next = parentInfo.first;
                int nextDir = parentInfo.second;
            
                // Prevent infinite loop by checking if we're revisiting the same point with same direction
                if (next == curr && nextDir == dir)
                {
                    break; // Stuck in loop, break
                }
                
                // Validate next point before continuing
                if (next.x >= -1 && next.y >= -1) // Allow (-1,-1) as termination condition
                {
                    curr = next;
                    dir = nextDir;
                }
                else
                {
                    break; // Invalid next point
                }
            }
            else
            {
                // Invalid parent index, break to prevent crash
                break;
            }
        }
        else
        {
            // Invalid indices, break to prevent crash
            break;
        }
    }
    std::reverse(path.begin(), path.end());
    return true;
}

// Draw the maze
void CMFCApplication2Dlg::DrawMaze(CDC* pDC)
{
    if (m_maze.empty()) return;

    CBrush* pOldBrush;

    // Ensure loop bounds match actual vector dimensions
    int actualRows = static_cast<int>(m_maze.size());
    for (int i = 0; i < actualRows && i < m_nRows; i++)
    {
        if (m_maze[i].empty()) continue;
        int actualCols = static_cast<int>(m_maze[i].size());
        for (int j = 0; j < actualCols && j < m_nCols; j++)
        {
            int x = m_drawRect.left + j * m_cellSize;
            int y = m_drawRect.top + i * m_cellSize;

            // Draw cell based on type
            CBrush brush;
            COLORREF color;
            switch (m_maze[i][j])
            {
            case EMPTY:
                color = RGB(255, 255, 255);
                break;
            case OBSTACLE:
                color = RGB(0, 0, 0);
                break;
            case START:
                color = RGB(0, 255, 0);
                break;
            case END:
                color = RGB(255, 0, 0);
                break;
            default:
                color = RGB(255, 255, 255);
                break;
            }

            if (brush.CreateSolidBrush(color))
            {
                pOldBrush = pDC->SelectObject(&brush);
                
                // Create and select a black pen for border
                CPen borderPen(PS_SOLID, 1, RGB(0, 0, 0));
                CPen* pOldPen = pDC->SelectObject(&borderPen);
                
                // Draw the cell with fill and border
                pDC->Rectangle(x, y, x + m_cellSize, y + m_cellSize);
                
                // Deselect objects
                pDC->SelectObject(pOldPen);
                pDC->SelectObject(pOldBrush);
            }
        }
    }
}

// Draw a path
void CMFCApplication2Dlg::DrawPath(CDC* pDC, const std::vector<Point>& path, COLORREF color)
{
    if (path.size() < 2) return;

    CPen pen(PS_SOLID, 3, color);
    CPen* pOldPen = pDC->SelectObject(&pen);

    for (size_t i = 0; i < path.size() - 1; i++)
    {
        Point p1 = path[i];
        Point p2 = path[i + 1];

        // Check if points are within maze bounds
        if (p1.x >= 0 && p1.x < m_nRows && p1.y >= 0 && p1.y < m_nCols &&
            p2.x >= 0 && p2.x < m_nRows && p2.y >= 0 && p2.y < m_nCols)
        {
            int x1 = m_drawRect.left + p1.y * m_cellSize + m_cellSize / 2;
            int y1 = m_drawRect.top + p1.x * m_cellSize + m_cellSize / 2;
            int x2 = m_drawRect.left + p2.y * m_cellSize + m_cellSize / 2;
            int y2 = m_drawRect.top + p2.x * m_cellSize + m_cellSize / 2;

            pDC->MoveTo(x1, y1);
            pDC->LineTo(x2, y2);
        }
    }

    pDC->SelectObject(pOldPen);
}

// Calculate optimal drawing rectangle and cell size to fit within dialog
void CMFCApplication2Dlg::CalculateDrawRect()
{
    CRect clientRect;
    GetClientRect(&clientRect);

    // Calculate available space below buttons (buttons at y=60 with height=20, so end at y=80)
    const int margin = 20;
    const int topMargin = 350; // Position below buttons with extra space (buttons end at y=80)
    
    // Debug output
    CString debugMsg;
    debugMsg.Format(L"CalculateDrawRect: topMargin=%d, clientRect.top=%d, clientRect.bottom=%d", 
                    topMargin, clientRect.top, clientRect.bottom);
    OutputDebugString(debugMsg);
    OutputDebugString(L"\n");
    const int maxCellSize = 50;
    const int minCellSize = 20;

    // Ensure we have at least 1 row and column to avoid division by zero
    int rows = max(m_nRows, 1);
    int cols = max(m_nCols, 1);

    // Calculate available width and height, ensuring it starts below all UI controls
    int availableWidth = clientRect.Width() - 2 * margin;
    int availableHeight = clientRect.Height() - topMargin - margin;

    // Ensure available space is at least the minimum required, but don't force it to be larger than client area
    availableWidth = max(availableWidth, 1);
    availableHeight = max(availableHeight, 1);

    // Calculate maximum possible cell size that fits both width and height
    int cellSizeForWidth = (availableWidth > 0 && cols > 0) ? availableWidth / cols : minCellSize;
    int cellSizeForHeight = (availableHeight > 0 && rows > 0) ? availableHeight / rows : minCellSize;

    // Choose the smaller of the two to ensure fit
    m_cellSize = min(min(cellSizeForWidth, cellSizeForHeight), maxCellSize);
    m_cellSize = max(m_cellSize, minCellSize); // Ensure minimum size

    // Calculate maze dimensions using actual maze size variables
    int mazeWidth = m_nCols * m_cellSize;
    int mazeHeight = m_nRows * m_cellSize;

    // Position maze below UI controls and center horizontally
    m_drawRect.left = margin + (clientRect.Width() - 2 * margin - mazeWidth) / 2;
    m_drawRect.top = topMargin;
    m_drawRect.right = m_drawRect.left + mazeWidth;
    m_drawRect.bottom = m_drawRect.top + mazeHeight;

    // Strictly ensure drawing rect doesn't overlap with UI controls and stays within client area
    m_drawRect.left = max(m_drawRect.left, margin);
    m_drawRect.top = max(m_drawRect.top, topMargin); // Critical: Ensure it starts below UI controls
    m_drawRect.right = min(m_drawRect.right, clientRect.right - margin);
    m_drawRect.bottom = min(m_drawRect.bottom, clientRect.bottom - margin);
    
    // Ensure drawRect is valid and has positive dimensions
    if (m_drawRect.left >= m_drawRect.right) {
        m_drawRect.left = margin;
        m_drawRect.right = clientRect.right - margin;
    }
    if (m_drawRect.top >= m_drawRect.bottom) {
        m_drawRect.top = topMargin;
        m_drawRect.bottom = topMargin + 100; // Minimum height
    }
    
    // Final safety check to ensure all values are within client area
    m_drawRect.NormalizeRect();
    m_drawRect.IntersectRect(m_drawRect, clientRect);
    
    // Ensure minimum size after all adjustments
    if (m_drawRect.Width() < 2) {
        m_drawRect.right = m_drawRect.left + 2;
    }
    if (m_drawRect.Height() < 2) {
        m_drawRect.bottom = m_drawRect.top + 2;
    }
    
    // Debug output for final drawRect
    CString finalMsg;
    finalMsg.Format(L"CalculateDrawRect: Final m_drawRect.top=%d, m_drawRect.bottom=%d, m_cellSize=%d", 
                    m_drawRect.top, m_drawRect.bottom, m_cellSize);
    OutputDebugString(finalMsg);
    OutputDebugString(L"\n");
}

// Draw the AGV
void CMFCApplication2Dlg::DrawAGV(CDC* pDC)
{
    if (m_path.empty() || m_maze.empty())
        return;

    // Check if AGV position is within maze bounds
    if (m_agvPosition.x < 0 || m_agvPosition.x >= m_nRows || 
        m_agvPosition.y < 0 || m_agvPosition.y >= m_nCols)
        return;

    int x = m_drawRect.left + m_agvPosition.y * m_cellSize + m_cellSize / 4;
    int y = m_drawRect.top + m_agvPosition.x * m_cellSize + m_cellSize / 4;

    CBrush brush(RGB(255, 255, 0));
    CBrush* pOldBrush = pDC->SelectObject(&brush);
    pDC->Ellipse(x, y, x + m_cellSize / 2, y + m_cellSize / 2);
    pDC->SelectObject(pOldBrush);
}

// Timer event handler
void CMFCApplication2Dlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_isRunning && !m_maze.empty() && !m_path.empty())
    {
        if (m_agvIndex < m_path.size())
        {
            // Ensure m_agvIndex is within bounds before accessing
            if (m_agvIndex >= 0 && m_agvIndex < m_path.size())
            {
                m_agvPosition = m_path[m_agvIndex];
                m_agvIndex++;
            }
            else
            {
                // Invalid index, stop timer
                m_isRunning = false;
                KillTimer(1);
                return;
            }

            // Check if reached the end, stop timer
            if (m_agvIndex >= m_path.size())
            {
                m_isRunning = false;
                KillTimer(1);
            }
            else
            {
                // Check if turning, adjust timer interval accordingly
                if (m_agvIndex < m_path.size() && m_agvIndex >= 2 && m_path.size() >= 2)
                {
                    // Ensure indices are within bounds
                    if ((m_agvIndex - 1) >= 0 && (m_agvIndex - 1) < m_path.size() && 
                        m_agvIndex >= 0 && m_agvIndex < m_path.size() && 
                        (m_agvIndex - 2) >= 0 && (m_agvIndex - 2) < m_path.size())
                    {
                        Point current = m_path[m_agvIndex - 1];
                        Point next = m_path[m_agvIndex];
                        
                        if (IsTurning(current, next))
                        {
                            // Turning requires additional time on top of the base step time
                            KillTimer(1);
                            SetTimer(1, m_speed + m_turnTime, nullptr);
                        }
                        else
                        {
                            // Straight movement uses normal speed
                            KillTimer(1);
                            SetTimer(1, m_speed, nullptr);
                        }
                    }
                }
            }
            
            InvalidateRect(&m_drawRect, FALSE);
        }
        else
        {
            // Ensure we stop properly
            m_isRunning = false;
            KillTimer(1);
        }
    }

    CDialogEx::OnTimer(nIDEvent);
}

// Start AGV movement
void CMFCApplication2Dlg::StartAGVMovement()
{
    if (m_path.empty())
        return;

    m_agvIndex = 0;
    m_agvPosition = m_startPoint;
    m_isRunning = true;
    SetTimer(1, m_speed, nullptr);
}

// Stop AGV movement
void CMFCApplication2Dlg::StopAGVMovement()
{
    m_isRunning = false;
    KillTimer(1);
}

// Handle window resize event
void CMFCApplication2Dlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);

    // Recalculate drawing area and cell size
    CalculateDrawRect();

    // Redraw the maze
    // Redraw the dialog with complete background clear to ensure proper resizing
    InvalidateRect(NULL, TRUE);
    UpdateWindow();
}

// Check if AGV is turning
bool CMFCApplication2Dlg::IsTurning(const Point& current, const Point& next)
{
    // Only check turning when we have previous position
    if (m_path.empty() || m_agvIndex < 2 || m_agvIndex >= m_path.size())
        return false;
    
    // Ensure m_agvIndex - 2 is within bounds
    if ((m_agvIndex - 2) < 0 || (m_agvIndex - 2) >= m_path.size())
        return false;
        
    Point prev = m_path[m_agvIndex - 2];
    Point curr = current;
    Point nxt = next;
    
    // Check if direction changes
    int prevDirX = curr.x - prev.x;
    int prevDirY = curr.y - prev.y;
    int nextDirX = nxt.x - curr.x;
    int nextDirY = nxt.y - curr.y;
    
    // If direction changes, it's a turn
    return (prevDirX != nextDirX) || (prevDirY != nextDirY);
}

// Generate maze button handler
void CMFCApplication2Dlg::OnBnClickedButtonGenerate()
{
    UpdateData(TRUE); // Get data from controls
    
    // Invalidate current drawing area to start with a clean slate
    InvalidateRect(NULL, TRUE);
    UpdateWindow();
    
    // Initialize new maze
    InitMaze(m_nRows, m_nCols);
    GenerateMaze();
    
    // Calculate paths
    Dijkstra(m_startPoint, m_endPoint, m_path);
    
    // Calculate fastest path considering turn time
    if (!CalculateFastestPath(m_startPoint, m_endPoint, m_fastestPath)) {
        // For large mazes, use the shortest path as fastest path
        m_fastestPath = m_path;
    }
    
    // Reset AGV position
    m_agvPosition = m_startPoint;
    
    // Calculate optimal drawing area and cell size
    CalculateDrawRect();
    
    // Redraw the dialog with complete background clear to ensure old map is removed
    InvalidateRect(NULL, TRUE);
    UpdateWindow(); // Force immediate redraw to avoid flickering
}

// Start simulation button handler
void CMFCApplication2Dlg::OnBnClickedButtonStart()
{
    UpdateData(TRUE); // Get data from controls
    StartAGVMovement();
}

// Stop simulation button handler
void CMFCApplication2Dlg::OnBnClickedButtonStop()
{
    StopAGVMovement();
}