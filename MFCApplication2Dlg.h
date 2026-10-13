
#pragma once

#include <vector>
#include <queue>
#include <algorithm>

enum CellType { EMPTY = 0, OBSTACLE = 1, START = 2, END = 3, PATH = 4, AGV = 5 };

struct Point {
    int x, y;
    Point() : x(0), y(0) {}
    Point(int x, int y) : x(x), y(y) {}
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }
};

struct DijkstraNode {
    Point point;
    int distance;
    Point parent;
    DijkstraNode() : distance(INT_MAX) {}
    DijkstraNode(Point p, int d, Point parent) : point(p), distance(d), parent(parent) {}
    bool operator<(const DijkstraNode& o) const { return distance > o.distance; }
};

class CMFCApplication2Dlg : public CDialogEx {
public:
    CMFCApplication2Dlg(CWnd* pParent = nullptr);
    virtual ~CMFCApplication2Dlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MFCAPPLICATION2_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    HICON m_hIcon;

    // Maze data
    int m_nRows, m_nCols;
    std::vector<std::vector<CellType>> m_maze;
    Point m_startPoint, m_endPoint;
    std::vector<Point> m_path, m_fastestPath;
    Point m_agvPosition;
    int m_agvIndex;
    bool m_isRunning;
    int m_speed, m_turnTime;
    
    // Drawing
    int m_cellSize;
    CRect m_drawRect;

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBnClickedButtonGenerate();
    afx_msg void OnBnClickedButtonStart();
    afx_msg void OnBnClickedButtonStop();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

    // Methods
    void InitMaze(int rows, int cols);
    void InitializeRandomStartEnd();
    void GenerateMaze();
    bool Dijkstra(Point start, Point end, std::vector<Point>& path);
    bool CalculateFastestPath(Point start, Point end, std::vector<Point>& path);
    bool IsValid(int x, int y);
    void DrawMaze(CDC* pDC);
    void DrawPath(CDC* pDC, const std::vector<Point>& path, COLORREF color);
    void DrawAGV(CDC* pDC);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    void StartAGVMovement();
    void StopAGVMovement();
    bool IsTurning(const Point& current, const Point& next);
    void CalculateDrawRect();
};
