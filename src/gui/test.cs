using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Effects;
using System.Windows.Shapes;

namespace PPSX33App
{
    public class Program
    {
        [STAThread]
        public static void Main()
        {
            App app = new App();
            app.Run(new MainWindow());
        }
    }

    public class App : Application { }

    #region Models & ViewModels
    public class LogEntry
    {
        public string Time { get; set; } = string.Empty;
        public string Level { get; set; } = string.Empty;
        public string Message { get; set; } = string.Empty;
        public Brush LevelColor { get; set; } = Brushes.Cyan;
    }

    public class MainViewModel : INotifyPropertyChanged
    {
        private bool _isBuilding = true;
        private double _progress = 89.7;
        private int _currentChunk = 149;
        private int _totalChunks = 166;
        private string _statusText = "Building... 149/166";
        private string _etaText = "45s";
        private string _activeTab = "Console";

        public ObservableCollection<LogEntry> Logs { get; } = new ObservableCollection<LogEntry>();

        public bool IsBuilding
        {
            get => _isBuilding;
            set { _isBuilding = value; OnPropertyChanged(); }
        }

        public double Progress
        {
            get => _progress;
            set { _progress = value; OnPropertyChanged(); }
        }

        public int CurrentChunk
        {
            get => _currentChunk;
            set { _currentChunk = value; OnPropertyChanged(); }
        }

        public string StatusText
        {
            get => _statusText;
            set { _statusText = value; OnPropertyChanged(); }
        }

        public string EtaText
        {
            get => _etaText;
            set { _etaText = value; OnPropertyChanged(); }
        }

        public string ActiveTab
        {
            get => _activeTab;
            set { _activeTab = value; OnPropertyChanged(); }
        }

        public MainViewModel()
        {
            // Seed initial compilation logs
            string[] initialLogs = new string[]
            {
                "ppu_chunk_122.cpp - COMPILING",
                "ppu_chunk_123.cpp - COMPILING",
                "ppu_chunk_124.cpp - COMPILING",
                "ppu_chunk_125.cpp - COMPILING",
                "ppu_chunk_126.cpp - COMPILING",
                "ppu_chunk_127.cpp - COMPILING",
                "ppu_chunk_128.cpp - COMPILING",
                "ppu_chunk_129.cpp - COMPILING",
                "ppu_chunk_130.cpp - COMPILING",
                "ppu_chunk_131.cpp - COMPILING",
                "ppu_chunk_132.cpp - COMPILING",
                "ppu_chunk_133.cpp - COMPILING",
                "ppu_chunk_134.cpp - COMPILING",
                "ppu_chunk_135.cpp - COMPILING",
                "ppu_chunk_136.cpp - COMPILING",
                "ppu_chunk_137.cpp - COMPILING",
                "ppu_chunk_138.cpp - COMPILING",
                "ppu_chunk_139.cpp - COMPILING",
                "ppu_chunk_140.cpp - COMPILING",
                "ppu_chunk_141.cpp - COMPILING"
            };

            foreach (var log in initialLogs)
            {
                AddLog("INFO", log);
            }

            StartSimulation();
        }

        public void AddLog(string level, string message)
        {
            Brush color = level switch
            {
                "SUCCESS" => new SolidColorBrush((Color)ColorConverter.ConvertFromString("#10B981")),
                "WARN" => new SolidColorBrush((Color)ColorConverter.ConvertFromString("#F59E0B")),
                "ERROR" => new SolidColorBrush((Color)ColorConverter.ConvertFromString("#EF4444")),
                _ => new SolidColorBrush((Color)ColorConverter.ConvertFromString("#38BDF8"))
            };

            Logs.Add(new LogEntry
            {
                Time = DateTime.Now.ToString("HH:mm:ss"),
                Level = level,
                Message = message,
                LevelColor = color
            });
        }

        private async void StartSimulation()
        {
            while (true)
            {
                await Task.Delay(1200);
                if (IsBuilding && CurrentChunk < _totalChunks)
                {
                    CurrentChunk++;
                    Progress = ((double)CurrentChunk / _totalChunks) * 100;
                    StatusText = $"Building... {CurrentChunk}/{_totalChunks}";
                    EtaText = $"{Math.Max(0, (166 - CurrentChunk) * 2)}s";

                    AddLog("INFO", $"COMPILE ppu_chunk_{CurrentChunk}.cpp -> OK");

                    if (CurrentChunk == _totalChunks)
                    {
                        IsBuilding = false;
                        StatusText = "Build Succeeded!";
                        EtaText = "0s";
                        AddLog("SUCCESS", "Recompilation completed successfully with 0 errors!");
                    }
                }
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
    #endregion

    #region GUI Window
    public class MainWindow : Window
    {
        private MainViewModel _vm;
        private ScrollViewer? _logScrollViewer;

        public MainWindow()
        {
            _vm = new MainViewModel();
            DataContext = _vm;

            Title = "PPSX33 - PlayStation 3 Recompiler - XXIII";
            Width = 1200;
            Height = 780;
            MinHeight = 650;
            MinWidth = 950;
            WindowStartupLocation = WindowStartupLocation.CenterScreen;
            Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#090D14"));

            Content = CreateUI();
        }

        private UIElement CreateUI()
        {
            Grid mainGrid = new Grid();
            mainGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(280) }); // Left Sidebar
            mainGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) }); // Workspace

            // -------------------------------------------------------------
            // LEFT SIDEBAR
            // -------------------------------------------------------------
            Border sidebarBorder = new Border
            {
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0D1117")),
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#1E293B")),
                BorderThickness = new Thickness(0, 0, 1, 0)
            };

            ScrollViewer sidebarScroll = new ScrollViewer { VerticalScrollBarVisibility = ScrollBarVisibility.Auto };
            StackPanel sidebar = new StackPanel { Margin = new Thickness(16) };

            // Logo Section with Glow Animation
            Grid logoGrid = new Grid { Margin = new Thickness(0, 0, 0, 20) };
            Ellipse outerRing = new Ellipse
            {
                Width = 90,
                Height = 90,
                Stroke = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0284C7")),
                StrokeThickness = 2,
                Effect = new DropShadowEffect { Color = (Color)ColorConverter.ConvertFromString("#0369A1"), BlurRadius = 15, ShadowDepth = 0 }
            };

            // Rotating animation for logo ring
            RotateTransform rotateTransform = new RotateTransform();
            outerRing.RenderTransform = rotateTransform;
            outerRing.RenderTransformOrigin = new Point(0.5, 0.5);
            DoubleAnimation rotationAnim = new DoubleAnimation(0, 360, TimeSpan.FromSeconds(10))
            {
                RepeatBehavior = RepeatBehavior.Forever
            };
            rotateTransform.BeginAnimation(RotateTransform.AngleProperty, rotationAnim);

            StackPanel logoContent = new StackPanel { VerticalAlignment = VerticalAlignment.Center, HorizontalAlignment = HorizontalAlignment.Center };
            TextBlock logoTitle = new TextBlock
            {
                Text = "PPSX33",
                FontSize = 20,
                FontWeight = FontWeights.Bold,
                Foreground = Brushes.White,
                HorizontalAlignment = HorizontalAlignment.Center
            };
            TextBlock logoSub = new TextBlock
            {
                Text = "PS3 RECOMPILER",
                FontSize = 9,
                FontWeight = FontWeights.SemiBold,
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#38BDF8")),
                HorizontalAlignment = HorizontalAlignment.Center
            };
            logoContent.Children.Add(logoTitle);
            logoContent.Children.Add(logoSub);

            logoGrid.Children.Add(outerRing);
            logoGrid.Children.Add(logoContent);
            sidebar.Children.Add(logoGrid);

            // WORKFLOW SECTION
            sidebar.Children.Add(CreateSectionHeader("WORKFLOW ACTIONS"));
            sidebar.Children.Add(CreateActionButton("Decrypt EBOOT", "#1E293B"));
            sidebar.Children.Add(CreateActionButton("Load ELF", "#1E293B"));
            sidebar.Children.Add(CreateActionButton("Decompile", "#1E293B"));
            sidebar.Children.Add(CreateActionButton("Build Project", "#0284C7", true));
            sidebar.Children.Add(CreateActionButton("Run Executable", "#1E293B"));
            sidebar.Children.Add(CreateActionButton("Copy to Game Folder", "#1E293B"));

            // Cancel Button
            Button cancelButton = CreateActionButton("Stop / Cancel Build", "#DC2626");
            cancelButton.Click += (s, e) =>
            {
                _vm.IsBuilding = false;
                _vm.StatusText = "Build Aborted by User";
                _vm.AddLog("WARN", "Build process cancelled!");
            };
            sidebar.Children.Add(cancelButton);

            // GRAPHICS & HARDWARE SETTINGS
            sidebar.Children.Add(CreateSectionHeader("GRAPHICS BACKEND", 16));

            ComboBox graphicsCombo = new ComboBox
            {
                Height = 36,
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#161B22")),
                Foreground = Brushes.White,
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#334155")),
                Margin = new Thickness(0, 4, 0, 12),
                Padding = new Thickness(8, 4, 8, 4)
            };

            ComboBoxItem itemVulkan = new ComboBoxItem { Content = "Vulkan (Recommended)", IsSelected = true, Foreground = Brushes.White };
            ComboBoxItem itemD3D11 = new ComboBoxItem { Content = "DirectX 11 (Recommended)", Foreground = Brushes.White };
            ComboBoxItem itemOpenGL = new ComboBoxItem { Content = "OpenGL (Unsupported / Disabled)", IsEnabled = false, Foreground = Brushes.Gray };

            graphicsCombo.Items.Add(itemVulkan);
            graphicsCombo.Items.Add(itemD3D11);
            graphicsCombo.Items.Add(itemOpenGL);
            sidebar.Children.Add(graphicsCombo);

            sidebar.Children.Add(CreateSectionHeader("CPU THREAD COUNT"));
            ComboBox threadsCombo = new ComboBox
            {
                Height = 36,
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#161B22")),
                Foreground = Brushes.White,
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#334155")),
                Margin = new Thickness(0, 4, 0, 12),
                Padding = new Thickness(8, 4, 8, 4)
            };
            threadsCombo.Items.Add(new ComboBoxItem { Content = "Auto Detect (16 Threads)", IsSelected = true });
            threadsCombo.Items.Add(new ComboBoxItem { Content = "8 Threads" });
            threadsCombo.Items.Add(new ComboBoxItem { Content = "4 Threads" });
            sidebar.Children.Add(threadsCombo);

            sidebarScroll.Content = sidebar;
            sidebarBorder.Child = sidebarScroll;
            Grid.SetColumn(sidebarBorder, 0);
            mainGrid.Children.Add(sidebarBorder);

            // -------------------------------------------------------------
            // WORKSPACE & CONSOLE AREA
            // -------------------------------------------------------------
            Grid workspaceGrid = new Grid();
            workspaceGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto }); // Path bar
            workspaceGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto }); // Tabs
            workspaceGrid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) }); // Main Console Output
            workspaceGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto }); // Bottom Progress Bar

            // Top Path Navigation Bar
            Border pathBorder = new Border
            {
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#161B22")),
                Padding = new Thickness(12, 10, 12, 10),
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#1E293B")),
                BorderThickness = new Thickness(0, 0, 0, 1)
            };
            Grid pathGrid = new Grid();
            pathGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
            pathGrid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            TextBlock pathLabel = new TextBlock
            {
                Text = "PROJECT PATH:",
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#64748B")),
                FontWeight = FontWeights.Bold,
                Margin = new Thickness(0, 0, 10, 0),
                VerticalAlignment = VerticalAlignment.Center
            };
            TextBox pathBox = new TextBox
            {
                Text = @"E:\PPSX33\build\dist\projects\GOW 3",
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#38BDF8")),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                IsReadOnly = true,
                FontFamily = new FontFamily("Consolas"),
                VerticalAlignment = VerticalAlignment.Center
            };
            Grid.SetColumn(pathLabel, 0);
            Grid.SetColumn(pathBox, 1);
            pathGrid.Children.Add(pathLabel);
            pathGrid.Children.Add(pathBox);
            pathBorder.Child = pathGrid;
            Grid.SetRow(pathBorder, 0);
            workspaceGrid.Children.Add(pathBorder);

            // TAB BAR SYSTEM
            StackPanel tabPanel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(16, 12, 16, 0)
            };
            tabPanel.Children.Add(CreateTabButton("Console", true));
            tabPanel.Children.Add(CreateTabButton("Report", false));
            tabPanel.Children.Add(CreateTabButton("Roadmap", false));
            tabPanel.Children.Add(CreateTabButton("Requirements", false));
            Grid.SetRow(tabPanel, 1);
            workspaceGrid.Children.Add(tabPanel);

            // MAIN CONSOLE TERMINAL
            Border terminalBorder = new Border
            {
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0D1117")),
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#1E293B")),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(8),
                Margin = new Thickness(16, 8, 16, 8)
            };

            _logScrollViewer = new ScrollViewer
            {
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                Padding = new Thickness(12)
            };

            ItemsControl logItemsControl = new ItemsControl();
            logItemsControl.SetBinding(ItemsControl.ItemsSourceProperty, new Binding("Logs"));

            // Log Template
            DataTemplate logTemplate = new DataTemplate(typeof(LogEntry));
            FrameworkElementFactory stackFactory = new FrameworkElementFactory(typeof(StackPanel));
            stackFactory.SetValue(StackPanel.OrientationProperty, Orientation.Horizontal);
            stackFactory.SetValue(StackPanel.MarginProperty, new Thickness(0, 2, 0, 2));

            // Time
            FrameworkElementFactory timeBlock = new FrameworkElementFactory(typeof(TextBlock));
            timeBlock.SetBinding(TextBlock.TextProperty, new Binding("Time") { StringFormat = "[{0}] " });
            timeBlock.SetValue(TextBlock.ForegroundProperty, new SolidColorBrush((Color)ColorConverter.ConvertFromString("#475569")));
            timeBlock.SetValue(TextBlock.FontFamilyProperty, new FontFamily("Consolas"));

            // Level
            FrameworkElementFactory levelBlock = new FrameworkElementFactory(typeof(TextBlock));
            levelBlock.SetBinding(TextBlock.TextProperty, new Binding("Level") { StringFormat = "{0}: " });
            levelBlock.SetBinding(TextBlock.ForegroundProperty, new Binding("LevelColor"));
            levelBlock.SetValue(TextBlock.FontWeightProperty, FontWeights.Bold);
            levelBlock.SetValue(TextBlock.FontFamilyProperty, new FontFamily("Consolas"));

            // Message
            FrameworkElementFactory msgBlock = new FrameworkElementFactory(typeof(TextBlock));
            msgBlock.SetBinding(TextBlock.TextProperty, new Binding("Message"));
            msgBlock.SetValue(TextBlock.ForegroundProperty, new SolidColorBrush((Color)ColorConverter.ConvertFromString("#E2E8F0")));
            msgBlock.SetValue(TextBlock.FontFamilyProperty, new FontFamily("Consolas"));

            stackFactory.AppendChild(timeBlock);
            stackFactory.AppendChild(levelBlock);
            stackFactory.AppendChild(msgBlock);
            logTemplate.VisualTree = stackFactory;

            logItemsControl.ItemTemplate = logTemplate;
            _logScrollViewer.Content = logItemsControl;

            // Auto Scroll
            _vm.Logs.CollectionChanged += (s, e) =>
            {
                _logScrollViewer.ScrollToEnd();
            };

            terminalBorder.Child = _logScrollViewer;
            Grid.SetRow(terminalBorder, 2);
            workspaceGrid.Children.Add(terminalBorder);

            // BOTTOM STATUS & PROGRESS BAR
            Border statusBorder = new Border
            {
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#161B22")),
                Padding = new Thickness(16, 10, 16, 12),
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#1E293B")),
                BorderThickness = new Thickness(0, 1, 0, 0)
            };

            Grid statusGrid = new Grid();
            statusGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            statusGrid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            Grid statusTextGrid = new Grid { Margin = new Thickness(0, 0, 0, 6) };
            TextBlock statusLabel = new TextBlock
            {
                Foreground = Brushes.White,
                FontWeight = FontWeights.SemiBold
            };
            statusLabel.SetBinding(TextBlock.TextProperty, new Binding("StatusText"));

            TextBlock etaLabel = new TextBlock
            {
                HorizontalAlignment = HorizontalAlignment.Right,
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#38BDF8")),
                FontWeight = FontWeights.Bold
            };
            etaLabel.SetBinding(TextBlock.TextProperty, new Binding("EtaText") { StringFormat = "ETA: {0}" });

            statusTextGrid.Children.Add(statusLabel);
            statusTextGrid.Children.Add(etaLabel);

            ProgressBar progressBar = new ProgressBar
            {
                Height = 8,
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0F172A")),
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0284C7")),
                BorderThickness = new Thickness(0)
            };
            progressBar.SetBinding(ProgressBar.ValueProperty, new Binding("Progress"));

            Grid.SetRow(statusTextGrid, 0);
            Grid.SetRow(progressBar, 1);
            statusGrid.Children.Add(statusTextGrid);
            statusGrid.Children.Add(progressBar);

            statusBorder.Child = statusGrid;
            Grid.SetRow(statusBorder, 3);
            workspaceGrid.Children.Add(statusBorder);

            Grid.SetColumn(workspaceGrid, 1);
            mainGrid.Children.Add(workspaceGrid);

            return mainGrid;
        }

        private TextBlock CreateSectionHeader(string text, double topMargin = 8)
        {
            return new TextBlock
            {
                Text = text,
                FontSize = 11,
                FontWeight = FontWeights.Bold,
                Foreground = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#64748B")),
                Margin = new Thickness(0, topMargin, 0, 6)
            };
        }

        private Button CreateActionButton(string title, string hexColor, bool isPrimary = false)
        {
            Button btn = new Button
            {
                Content = title,
                Height = 38,
                Margin = new Thickness(0, 3, 0, 3),
                Foreground = Brushes.White,
                FontWeight = isPrimary ? FontWeights.Bold : FontWeights.Normal,
                Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString(hexColor)),
                BorderThickness = new Thickness(1),
                BorderBrush = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#334155")),
                Cursor = System.Windows.Input.Cursors.Hand
            };

            // Hover effect animation
            btn.MouseEnter += (s, e) => btn.Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#0284C7"));
            btn.MouseLeave += (s, e) => btn.Background = new SolidColorBrush((Color)ColorConverter.ConvertFromString(hexColor));

            return btn;
        }

        private Button CreateTabButton(string header, bool isActive)
        {
            Button tab = new Button
            {
                Content = header,
                Padding = new Thickness(16, 6, 16, 6),
                Margin = new Thickness(0, 0, 8, 0),
                Foreground = isActive ? Brushes.White : new SolidColorBrush((Color)ColorConverter.ConvertFromString("#64748B")),
                Background = isActive ? new SolidColorBrush((Color)ColorConverter.ConvertFromString("#1E293B")) : Brushes.Transparent,
                BorderBrush = isActive ? new SolidColorBrush((Color)ColorConverter.ConvertFromString("#38BDF8")) : Brushes.Transparent,
                BorderThickness = isActive ? new Thickness(0, 0, 0, 2) : new Thickness(0),
                FontWeight = isActive ? FontWeights.Bold : FontWeights.Normal,
                Cursor = System.Windows.Input.Cursors.Hand
            };

            tab.Click += (s, e) =>
            {
                _vm.ActiveTab = header;
                _vm.AddLog("INFO", $"Switched view to tab: {header}");
            };

            return tab;
        }
    }
    #endregion
}