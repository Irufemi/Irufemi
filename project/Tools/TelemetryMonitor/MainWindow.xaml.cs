using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace TelemetryMonitor
{
    public class MetricItem : System.ComponentModel.INotifyPropertyChanged
    {
        public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;
        public string Key { get; set; } = string.Empty;
        
        private string _value = string.Empty;
        public string Value
        {
            get => _value;
            set
            {
                if (_value != value)
                {
                    _value = value;
                    PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(nameof(Value)));
                    UpdateStatusColor();
                }
            }
        }

        private Brush _statusColorBrush = new SolidColorBrush(Color.FromRgb(255, 255, 255));
        public Brush StatusColorBrush
        {
            get => _statusColorBrush;
            set
            {
                if (_statusColorBrush != value)
                {
                    _statusColorBrush = value;
                    PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(nameof(StatusColorBrush)));
                }
            }
        }

        private void UpdateStatusColor()
        {
            // ターゲットFPSをとりあえず60 (1フレームの限界 = 16.66ms) とする
            double targetFrameTime = 16.66;
            
            if (double.TryParse(Value, out double dVal))
            {
                if (Key.Contains("FPS", StringComparison.OrdinalIgnoreCase))
                {
                    // FPS: 高いほど良い
                    if (dVal >= 58.0) StatusColorBrush = new SolidColorBrush(Color.FromRgb(0, 255, 157)); // Green
                    else if (dVal >= 30.0) StatusColorBrush = new SolidColorBrush(Color.FromRgb(255, 204, 0)); // Yellow
                    else StatusColorBrush = new SolidColorBrush(Color.FromRgb(255, 51, 102)); // Red
                }
                else if (Key.Contains("Time", StringComparison.OrdinalIgnoreCase) || Key.Contains("CPU", StringComparison.OrdinalIgnoreCase) || Key.Contains("GPU", StringComparison.OrdinalIgnoreCase))
                {
                    // 処理時間(ms): 低いほど良い。16.66msが限界。
                    if (dVal < targetFrameTime * 0.6) StatusColorBrush = new SolidColorBrush(Color.FromRgb(0, 255, 157)); // 余裕 (Green)
                    else if (dVal <= targetFrameTime) StatusColorBrush = new SolidColorBrush(Color.FromRgb(255, 204, 0)); // 限界に近い (Yellow)
                    else StatusColorBrush = new SolidColorBrush(Color.FromRgb(255, 51, 102)); // 限界突破/処理落ち (Red)
                }
                else
                {
                    StatusColorBrush = new SolidColorBrush(Color.FromRgb(255, 255, 255)); // White
                }
            }
        }
    }

    public partial class MainWindow : Window
    {
        private ObservableCollection<MetricItem> _metrics = new ObservableCollection<MetricItem>();
        private UdpClient? _udpClient;

        // グラフ描画用履歴
        private const int MaxHistory = 300;
        private Queue<double> _fpsHistory = new Queue<double>();
        private Queue<double> _cpuHistory = new Queue<double>();
        private Queue<double> _gpuHistory = new Queue<double>();

        public MainWindow()
        {
            InitializeComponent();
            MetricsList.ItemsSource = _metrics;
            StartListening();
        }

        private void StartListening()
        {
            try
            {
                _udpClient = new UdpClient(8888);
                Task.Run(ReceiveDataAsync);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to start UDP listener on port 8888:\n{ex.Message}");
            }
        }

        private async Task ReceiveDataAsync()
        {
            if (_udpClient == null) return;

            while (true)
            {
                try
                {
                    var result = await _udpClient.ReceiveAsync();
                    string json = Encoding.UTF8.GetString(result.Buffer);

                    using (JsonDocument doc = JsonDocument.Parse(json))
                    {
                        var root = doc.RootElement;
                        Dispatcher.Invoke(() =>
                        {
                            foreach (var property in root.EnumerateObject())
                            {
                                string key = property.Name;
                                string valueStr;

                                if (property.Value.ValueKind == JsonValueKind.Number && property.Value.TryGetDouble(out double dVal))
                                {
                                    valueStr = dVal.ToString("0.00");
                                    
                                    // グラフ用の履歴に追加
                                    if (key.Contains("FPS", StringComparison.OrdinalIgnoreCase))
                                        AddHistory(_fpsHistory, dVal);
                                    else if (key.Contains("CPU", StringComparison.OrdinalIgnoreCase))
                                        AddHistory(_cpuHistory, dVal);
                                    else if (key.Contains("GPU", StringComparison.OrdinalIgnoreCase))
                                        AddHistory(_gpuHistory, dVal);
                                }
                                else if (property.Value.ValueKind == JsonValueKind.Array)
                                {
                                    valueStr = string.Join(", ", property.Value.EnumerateArray().Select(e => e.ToString()));
                                }
                                else
                                {
                                    valueStr = property.Value.ToString();
                                }

                                var existing = _metrics.FirstOrDefault(m => m.Key == key);
                                if (existing != null)
                                {
                                    existing.Value = valueStr;
                                }
                                else
                                {
                                    _metrics.Add(new MetricItem { Key = key, Value = valueStr });
                                }
                            }

                            // フレームの最後にグラフを更新
                            UpdateGraph();
                        });
                    }
                }
                catch (Exception)
                {
                    // Ignore parse errors or socket closures
                }
            }
        }

        private void AddHistory(Queue<double> queue, double value)
        {
            queue.Enqueue(value);
            if (queue.Count > MaxHistory) queue.Dequeue();
        }

        private void UpdateGraph()
        {
            double width = GraphCanvas.ActualWidth;
            double height = GraphCanvas.ActualHeight;
            if (width <= 0 || height <= 0) return;

            // スケール最大値（FPSは120、CPU/GPU時間はとりあえず33.3ms = 約30FPS基準）
            UpdatePolylineAndPolygon(FpsLine, FpsArea, _fpsHistory, width, height, 120.0);
            UpdatePolylineAndPolygon(CpuLine, CpuArea, _cpuHistory, width, height, 33.3);
            UpdatePolylineAndPolygon(GpuLine, GpuArea, _gpuHistory, width, height, 33.3);
        }

        private void UpdatePolylineAndPolygon(System.Windows.Shapes.Polyline polyline, System.Windows.Shapes.Polygon polygon, Queue<double> history, double width, double height, double maxValue)
        {
            var points = new PointCollection();
            int count = history.Count;
            if (count == 0) return;

            double step = width / (MaxHistory - 1);
            double startX = width - (count * step);

            int i = 0;
            foreach (var val in history)
            {
                double x = startX + (i * step);
                double normalized = Math.Min(val / maxValue, 1.0);
                normalized = Math.Max(normalized, 0.0);
                double y = height - (normalized * height);
                
                points.Add(new Point(x, y));
                i++;
            }
            
            // 折れ線用
            polyline.Points = points;
            
            // エリアグラフ（Polygon）用：始点と終点を下部(height)に落として閉じた図形にする
            var areaPoints = new PointCollection(points);
            if (points.Count > 0)
            {
                areaPoints.Add(new Point(points.Last().X, height)); // 最後の点から真下へ
                areaPoints.Add(new Point(points.First().X, height)); // 最初の点の真下へ
            }
            polygon.Points = areaPoints;
        }

        private void GraphCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateGraph();
        }

        protected override void OnClosed(EventArgs e)
        {
            _udpClient?.Close();
            base.OnClosed(e);
        }
    }
}