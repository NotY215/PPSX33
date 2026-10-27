using System.Drawing.Drawing2D;
using System.Text.Json;

namespace PS3Recomp.Gui;

/// <summary>
/// Fully native, DPI-scaled DualShock-style mapper. No browser or external assets required.
/// </summary>
public sealed class ControllerControl : Control
{
    private const int W = 1200, H = 900;
    private static readonly Color Bg = Color.FromArgb(4, 8, 16);
    private static readonly Color Cyan = Color.FromArgb(0, 229, 255);
    private static readonly Color Border = Color.FromArgb(29, 52, 89);
    private static readonly Color White = Color.FromArgb(235, 245, 255);
    private static readonly string[] Order = {
        "dpad_up","dpad_down","dpad_left","dpad_right","triangle","circle","cross","square",
        "l1","r1","l2","r2","select","start","ps",
        "ls_up","ls_down","ls_left","ls_right","rs_up","rs_down","rs_left","rs_right","l3","r3"
    };
    private static readonly Dictionary<string,string> Names = new() {
        ["dpad_up"]="D-Pad Up",["dpad_down"]="D-Pad Down",["dpad_left"]="D-Pad Left",["dpad_right"]="D-Pad Right",
        ["triangle"]="Triangle",["circle"]="Circle",["cross"]="Cross",["square"]="Square",
        ["l1"]="L1",["r1"]="R1",["l2"]="L2",["r2"]="R2",["select"]="SELECT",["start"]="START",["ps"]="PS",
        ["ls_up"]="Left Stick ↑",["ls_down"]="Left Stick ↓",["ls_left"]="Left Stick ←",["ls_right"]="Left Stick →",
        ["rs_up"]="Right Stick ↑",["rs_down"]="Right Stick ↓",["rs_left"]="Right Stick ←",["rs_right"]="Right Stick →",
        ["l3"]="L3 · Stick Click",["r3"]="R3 · Stick Click"
    };
    private readonly Dictionary<string,RectangleF> _hits = new();
    private readonly HashSet<string> _held = new(StringComparer.OrdinalIgnoreCase);
    private readonly System.Windows.Forms.Timer _timer;
    private KeyMap _map;
    private string? _rebinding, _hover;
    private float _phase, _leftX, _leftY, _rightX, _rightY;
    private bool _savedFlash;

    public event EventHandler? MapChanged;
    public event EventHandler? SaveRequested;
    public KeyMap CurrentMap => JsonSerializer.Deserialize<KeyMap>(JsonSerializer.Serialize(_map)) ?? KeyMap.Defaults();

    public ControllerControl(KeyMap map)
    {
        _map = JsonSerializer.Deserialize<KeyMap>(JsonSerializer.Serialize(map)) ?? KeyMap.Defaults();
        BackColor = Bg; ForeColor = White; Dock = DockStyle.Fill;
        TabStop = true;
        SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw, true);
        _timer = new System.Windows.Forms.Timer { Interval = 33 };
        _timer.Tick += (_, _) => {
            _phase = (_phase + .075f) % (float)(Math.PI * 2);
            float lx = Direction("ls_left") * -1f + Direction("ls_right");
            float ly = Direction("ls_up") * -1f + Direction("ls_down");
            float rx = Direction("rs_left") * -1f + Direction("rs_right");
            float ry = Direction("rs_up") * -1f + Direction("rs_down");
            _leftX = Ease(_leftX, lx * 17f); _leftY = Ease(_leftY, ly * 17f);
            _rightX = Ease(_rightX, rx * 17f); _rightY = Ease(_rightY, ry * 17f);
            Invalidate();
        };
        _timer.Start();
        Font = new Font("Segoe UI", 9f);
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing) { _timer.Stop(); _timer.Dispose(); }
        base.Dispose(disposing);
    }

    private static float Ease(float a, float b) => a + (b-a)*.28f;
    private float Direction(string id) => _held.Contains(GetKey(id)) ? 1f : 0f;
    private string GetKey(string id) => Get(id);
    protected override bool IsInputKey(Keys keyData) => true;

    protected override void OnKeyDown(KeyEventArgs e)
    {
        base.OnKeyDown(e);
        if (_rebinding != null)
        {
            if (e.KeyCode == Keys.Escape) { _rebinding = null; Invalidate(); }
            else if (e.KeyCode != Keys.ControlKey && e.KeyCode != Keys.ShiftKey && e.KeyCode != Keys.Menu)
            {
                Set(_rebinding, NormalizeKey(e.KeyCode));
                _rebinding = null; MapChanged?.Invoke(this, EventArgs.Empty); Invalidate();
            }
            e.Handled = true; e.SuppressKeyPress = true; return;
        }
        _held.Add(NormalizeKey(e.KeyCode));
    }
    protected override void OnKeyUp(KeyEventArgs e) { base.OnKeyUp(e); _held.Remove(NormalizeKey(e.KeyCode)); }
    protected override void OnMouseMove(MouseEventArgs e)
    {
        base.OnMouseMove(e);
        string? next = Hit(e.Location);
        if (_hover != next) { _hover = next; Cursor = next == null ? Cursors.Default : Cursors.Hand; Invalidate(); }
    }
    protected override void OnMouseLeave(EventArgs e) { base.OnMouseLeave(e); _hover = null; Cursor = Cursors.Default; Invalidate(); }
    protected override void OnMouseDown(MouseEventArgs e)
    {
        base.OnMouseDown(e); Focus();
        string? id = Hit(e.Location);
        if (id == null) return;
        if (id.StartsWith("@"))
        {
            switch (id)
            {
                case "@reset":
                    if (MessageBox.Show(FindForm(), "Restore every controller binding to the PPSX33 defaults?", "Reset key mapping", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                    { _map = KeyMap.Defaults(); _rebinding = null; MapChanged?.Invoke(this, EventArgs.Empty); Invalidate(); }
                    break;
                case "@save": SaveRequested?.Invoke(this, EventArgs.Empty); _savedFlash = true; Invalidate(); break;
                case "@import": ImportMap(); break;
                case "@export": ExportMap(); break;
            }
            return;
        }
        _rebinding = id; Invalidate();
    }

    private string? Hit(Point p)
    {
        float scale = Math.Min(ClientSize.Width / (float)W, ClientSize.Height / (float)H);
        if (scale <= 0) return null;
        float ox = (ClientSize.Width - W * scale) * .5f;
        float oy = (ClientSize.Height - H * scale) * .5f;
        PointF q = new((p.X - ox) / scale, (p.Y - oy) / scale);
        foreach (var kv in _hits) if (kv.Value.Contains(q)) return kv.Key;
        return null;
    }
    private static string NormalizeKey(Keys key) => key switch {
        Keys.ControlKey => "ControlKey", Keys.ShiftKey => "ShiftKey", Keys.Menu => "Menu",
        Keys.Return => "Enter", Keys.Prior => "PageUp", Keys.Next => "PageDown",
        _ => key.ToString()
    };
    private static string DisplayKey(string key) => key switch {
        "RControlKey"=>"R Ctrl","LControlKey"=>"L Ctrl","RShiftKey"=>"R Shift","LShiftKey"=>"L Shift",
        "ControlKey"=>"Ctrl","ShiftKey"=>"Shift","NumPad0"=>"Num 0","NumPad1"=>"Num 1","NumPad2"=>"Num 2",
        "NumPad3"=>"Num 3","NumPad4"=>"Num 4","NumPad5"=>"Num 5","NumPad6"=>"Num 6","NumPad7"=>"Num 7",
        "NumPad8"=>"Num 8","NumPad9"=>"Num 9","Space"=>"Space","Backspace"=>"Backspace","Enter"=>"Enter",
        "Left"=>"←","Right"=>"→","Up"=>"↑","Down"=>"↓", _ => key
    };
    private string Get(string id)
    {
        return id switch {
            "dpad_up"=>_map.DpadUp,"dpad_down"=>_map.DpadDown,"dpad_left"=>_map.DpadLeft,"dpad_right"=>_map.DpadRight,
            "triangle"=>_map.Triangle,"circle"=>_map.Circle,"cross"=>_map.Cross,"square"=>_map.Square,
            "l1"=>_map.L1,"r1"=>_map.R1,"l2"=>_map.L2,"r2"=>_map.R2,"select"=>_map.Select,"start"=>_map.Start,"ps"=>_map.PsButton,
            "ls_up"=>_map.LeftStickUp,"ls_down"=>_map.LeftStickDown,"ls_left"=>_map.LeftStickLeft,"ls_right"=>_map.LeftStickRight,
            "rs_up"=>_map.RightStickUp,"rs_down"=>_map.RightStickDown,"rs_left"=>_map.RightStickLeft,"rs_right"=>_map.RightStickRight,
            "l3"=>_map.L3,"r3"=>_map.R3,_=>string.Empty
        };
    }
    private void Set(string id,string value)
    {
        switch(id) {
            case "dpad_up":_map.DpadUp=value;break; case "dpad_down":_map.DpadDown=value;break;
            case "dpad_left":_map.DpadLeft=value;break; case "dpad_right":_map.DpadRight=value;break;
            case "triangle":_map.Triangle=value;break; case "circle":_map.Circle=value;break;
            case "cross":_map.Cross=value;break; case "square":_map.Square=value;break;
            case "l1":_map.L1=value;break; case "r1":_map.R1=value;break; case "l2":_map.L2=value;break; case "r2":_map.R2=value;break;
            case "select":_map.Select=value;break; case "start":_map.Start=value;break; case "ps":_map.PsButton=value;break;
            case "ls_up":_map.LeftStickUp=value;break; case "ls_down":_map.LeftStickDown=value;break;
            case "ls_left":_map.LeftStickLeft=value;break; case "ls_right":_map.LeftStickRight=value;break;
            case "rs_up":_map.RightStickUp=value;break; case "rs_down":_map.RightStickDown=value;break;
            case "rs_left":_map.RightStickLeft=value;break; case "rs_right":_map.RightStickRight=value;break;
            case "l3":_map.L3=value;break; case "r3":_map.R3=value;break;
        }
    }
    private void ImportMap()
    {
        using var d = new OpenFileDialog { Filter="JSON key maps (*.json)|*.json|All files (*.*)|*.*", Title="Import PPSX33 key map" };
        if (d.ShowDialog(FindForm()) != DialogResult.OK) return;
        try { _map = JsonSerializer.Deserialize<KeyMap>(File.ReadAllText(d.FileName)) ?? throw new InvalidDataException("The file did not contain a key map."); MapChanged?.Invoke(this, EventArgs.Empty); Invalidate(); }
        catch(Exception ex) { MessageBox.Show(FindForm(), "Could not import this key map.\n"+ex.Message, "Import failed", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }
    private void ExportMap()
    {
        using var d = new SaveFileDialog { Filter="JSON key maps (*.json)|*.json", FileName="ppsx33-key-map.json", Title="Export PPSX33 key map" };
        if (d.ShowDialog(FindForm()) != DialogResult.OK) return;
        try { File.WriteAllText(d.FileName, JsonSerializer.Serialize(_map, new JsonSerializerOptions { WriteIndented=true })); }
        catch(Exception ex) { MessageBox.Show(FindForm(), "Could not export this key map.\n"+ex.Message, "Export failed", MessageBoxButtons.OK, MessageBoxIcon.Error); }
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        float scale = Math.Min(ClientSize.Width / (float)W, ClientSize.Height / (float)H);
        if (scale <= 0) return;
        float ox = (ClientSize.Width - W * scale) * .5f;
        float oy = (ClientSize.Height - H * scale) * .5f;

        // Render the entire UI in its native 1200x900 coordinate space first.
        // TextRenderer draws in device coordinates and ignores Graphics transforms;
        // drawing to a virtual canvas prevents labels from drifting away from buttons.
        using var canvas = new Bitmap(W, H);
        using (var g = Graphics.FromImage(canvas))
        {
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.InterpolationMode = InterpolationMode.HighQualityBicubic;
            g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
            g.Clear(Bg);
            using (var b = new SolidBrush(Bg)) g.FillRectangle(b, 0, 0, W, H);
            using (var glow = new SolidBrush(Color.FromArgb(18, 0, 229, 255))) g.FillEllipse(glow, 280, -210, 640, 500);
            using (var grid = new Pen(Color.FromArgb(18, 29, 52, 89), 1))
            {
                for (int x = 0; x < W; x += 32) g.DrawLine(grid, x, 0, x, H);
                for (int y = 0; y < H; y += 32) g.DrawLine(grid, 0, y, W, y);
            }
            _hits.Clear();
            DrawHeader(g);
            DrawController(g);
            DrawMappingGrid(g);
            if (_rebinding != null) DrawRebindOverlay(g);
        }

        e.Graphics.SmoothingMode = SmoothingMode.HighQuality;
        e.Graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
        e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
        e.Graphics.DrawImage(canvas, new RectangleF(ox, oy, W * scale, H * scale));
    }

    private void DrawHeader(Graphics g)
    {
        using var title=new Font("Segoe UI",28,FontStyle.Bold|FontStyle.Italic);
        using var sub=new Font("Segoe UI",10,FontStyle.Bold);
        using var small=new Font("Segoe UI",9);
        using(var b=new LinearGradientBrush(new Rectangle(28,18,230,45),Color.FromArgb(125,220,255),Cyan,0f))
            g.DrawString("PPSX33",title,b,28,10);
        using(var divider=new Pen(Border,2))g.DrawLine(divider,260,20,260,65);
        using var subBrush=new SolidBrush(White);
        using var smallBrush=new SolidBrush(Color.FromArgb(102,190,220));
        g.DrawString("PLAYSTATION 3  /  CONTROLLER STUDIO",sub,subBrush,280,20);
        g.DrawString("NATIVE INPUT MAPPER  •  CLICK ANY CONTROL TO REBIND",small,smallBrush,281,43);
        DrawToolbar(g,"@import","IMPORT",28,76,130);
        DrawToolbar(g,"@export","EXPORT",168,76,130);
        DrawToolbar(g,"@reset","RESET DEFAULTS",308,76,180);
        DrawToolbar(g,"@save",_savedFlash?"SAVED ✓":"SAVE MAP",W-192,76,164);
        using var line=new Pen(Color.FromArgb(100,Border),1);g.DrawLine(line,28,118,W-28,118);
    }
    private void DrawToolbar(Graphics g,string id,string text,int x,int y,int w)
    {
        var r=new RectangleF(x,y,w,31); _hits[id]=r;
        bool hot=_hover==id;
        using(var b=new LinearGradientBrush(Rectangle.Round(r),hot?Color.FromArgb(18,57,84):Color.FromArgb(9,18,31),Color.FromArgb(5,12,23),90f))g.FillRounded(b,r,7);
        using(var p=new Pen(hot?Cyan:Border,hot?1.5f:1f))g.DrawRounded(p,r,7);
        using var f=new Font("Segoe UI",8.5f,FontStyle.Bold);
        TextRenderer.DrawText(g,text,f,Rectangle.Round(r),hot?Cyan:White,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }

    private void DrawController(Graphics g)
    {
        using var caption=new Font("Segoe UI",8,FontStyle.Bold);
        using var captionBrush=new SolidBrush(Color.FromArgb(95,180,210));
        g.DrawString("DUALSHOCK-STYLE CONTROLLER",caption,captionBrush,456,130);
        // soft cast shadow
        using(var sh=new SolidBrush(Color.FromArgb(125,0,0,0)))g.FillEllipse(sh,260,425,680,46);
        // grip silhouettes
        using(var grip=new LinearGradientBrush(new Rectangle(240,250,720,220),Color.FromArgb(30,47,72),Color.FromArgb(7,14,25),90f))
        using(var p=new Pen(Color.FromArgb(55,90,130),2))
        using(var path=GripPath()) { g.FillPath(grip,path);g.DrawPath(p,path); }
        // body shell
        using(var body=BodyPath())
        using(var fill=new LinearGradientBrush(new Rectangle(250,160,700,300),Color.FromArgb(31,45,67),Color.FromArgb(7,13,24),90f))
        using(var outline=new Pen(Color.FromArgb(55,87,126),2.4f)) { g.FillPath(fill,body);g.DrawPath(outline,body); }
        using(var rim=new Pen(Color.FromArgb(65,98,137),1f))
        using(var path=BodyPath(8))g.DrawPath(rim,path);
        // grip grooves and small surface highlights
        using(var groove=new Pen(Color.FromArgb(28,100,140,180),2))
        { for(int i=0;i<5;i++){g.DrawArc(groove,265+i*10,340,105,125,35,90);g.DrawArc(groove,830+i*10,340,105,125,55,90);} }
        // shoulders and triggers
        DrawShoulder(g,"l2","L2",334,160,95,31); DrawShoulder(g,"l1","L1",344,190,78,23);
        DrawShoulder(g,"r2","R2",771,160,95,31); DrawShoulder(g,"r1","R1",778,190,78,23);
        // center panel, speaker holes, LEDs, tiny screws
        using(var p=CenterPlate())
        using(var b=new LinearGradientBrush(new Rectangle(440,214,320,112),Color.FromArgb(20,33,53),Color.FromArgb(7,14,25),90f))
        using(var pen=new Pen(Color.FromArgb(48,75,110),1.2f)) { g.FillPath(b,p); g.DrawPath(pen,p); }
        for(int row=0;row<3;row++)for(int col=0;col<7;col++){using var b=new SolidBrush(Color.FromArgb(70,105,132,160));g.FillEllipse(b,570+col*9,250+row*8,3,3);}
        for(int i=0;i<4;i++){using var led=new SolidBrush(i==0?Color.FromArgb(150,0,229,255):Color.FromArgb(35,110,140,165));g.FillEllipse(led,588+i*10,300,5,5);}
        foreach(var pt in new[]{new PointF(330,223),new PointF(870,223),new PointF(386,397),new PointF(814,397)}){using var b=new SolidBrush(Color.FromArgb(130,4,8,16));g.FillEllipse(b,pt.X-3,pt.Y-3,6,6);using var p=new Pen(Color.FromArgb(60,125,150,180));g.DrawEllipse(p,pt.X-3,pt.Y-3,6,6);}
        // D-pad
        DrawDpad(g);
        // face buttons with original PlayStation colours
        DrawFace(g,"triangle", "△",Color.FromArgb(0,230,118),824,264);
        DrawFace(g,"circle", "○",Color.FromArgb(255,82,82),870,307);
        DrawFace(g,"cross", "×",Color.FromArgb(64,196,255),824,350);
        DrawFace(g,"square", "□",Color.FromArgb(224,64,251),778,307);
        // analog wells and animated caps
        DrawStick(g,"ls", "L3",405,337,_leftX,_leftY);
        DrawStick(g,"rs", "R3",695,337,_rightX,_rightY);
        // select / start / PS controls
        DrawPill(g,"select","SELECT",505,345,66,23);
        DrawPill(g,"start","START",629,345,66,23);
        DrawPs(g);
        // click zones for stick directions
        AddHit("ls_left",355,310,39,57); AddHit("ls_right",436,310,39,57); AddHit("ls_up",390,289,48,30); AddHit("ls_down",390,365,48,28);
        AddHit("rs_left",645,310,39,57); AddHit("rs_right",726,310,39,57); AddHit("rs_up",680,289,48,30); AddHit("rs_down",680,365,48,28);
        // directional caps move in response to the corresponding keyboard binding
        DrawStickDirectionLabel(g,"ls",405,337); DrawStickDirectionLabel(g,"rs",695,337);
    }
    private static GraphicsPath GripPath()
    {
        var p=new GraphicsPath();p.AddBezier(315,250,270,265,250,335,275,410);p.AddBezier(275,410,300,480,365,487,390,431);
        p.AddBezier(390,431,410,390,390,312,365,278);p.AddBezier(365,278,350,258,330,250,315,250);
        p.AddBezier(885,250,930,265,950,335,925,410);p.AddBezier(925,410,900,480,835,487,810,431);
        p.AddBezier(810,431,790,390,810,312,835,278);p.AddBezier(835,278,850,258,870,250,885,250);return p;
    }
    private static GraphicsPath BodyPath(float inset=0)
    {
        float d=inset;var p=new GraphicsPath();
        p.AddBezier(330+d,184+d,385+d,145+d,470+d,145+d,600,151+d);
        p.AddBezier(600,151+d,730-d,145+d,815-d,145+d,870-d,184+d);
        p.AddBezier(870-d,184+d,925-d,222+d,927-d,281,902-d,330);
        p.AddBezier(902-d,330,883-d,370,852-d,389,807-d,391-d);
        p.AddBezier(807-d,391-d,754-d,427-d,698-d,436-d,600,436-d);
        p.AddBezier(600,436-d,502+d,436-d,446+d,427-d,393+d,391-d);
        p.AddBezier(393+d,391-d,348+d,389,317+d,370,298+d,330);
        p.AddBezier(298+d,330,273+d,281,275+d,222+d,330+d,184+d);p.CloseFigure();return p;
    }
    private static GraphicsPath CenterPlate(){var p=new GraphicsPath();p.AddBezier(464,222,500,204,700,204,736,222);p.AddBezier(736,222,750,250,750,285,733,310);p.AddBezier(733,310,690,328,510,328,467,310);p.AddBezier(467,310,450,285,450,250,464,222);p.CloseFigure();return p;}
    private void DrawShoulder(Graphics g,string id,string label,float x,float y,float w,float h)
    {
        AddHit(id,x,y,w,h);bool hot=_hover==id||_rebinding==id;
        using(var b=new LinearGradientBrush(new RectangleF(x,y,w,h),hot?Color.FromArgb(20,99,130):Color.FromArgb(28,48,72),Color.FromArgb(7,15,28),90f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(50,83,120),hot?2:1.3f)){g.FillRounded(b,new RectangleF(x,y,w,h),7);g.DrawRounded(p,new RectangleF(x,y,w,h),7);}
        using var f=new Font("Segoe UI",8,FontStyle.Bold);TextRenderer.DrawText(g,label,f,Rectangle.Round(new RectangleF(x,y,w,h)),hot?Cyan:Color.FromArgb(135,183,211),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void DrawDpad(Graphics g)
    {
        float x=344,y=258;DrawDpadPiece(g,"dpad_up",x+25,y,29,32,"▲");DrawDpadPiece(g,"dpad_down",x+25,y+59,29,32,"▼");
        DrawDpadPiece(g,"dpad_left",x,y+30,32,29,"◀");DrawDpadPiece(g,"dpad_right",x+47,y+30,32,29,"▶");
        using(var b=new SolidBrush(Color.FromArgb(8,16,28)))g.FillEllipse(b,x+27,y+31,27,27);
        using(var p=new Pen(Color.FromArgb(35,75,108),1))g.DrawEllipse(p,x+27,y+31,27,27);
    }
    private void DrawDpadPiece(Graphics g,string id,float x,float y,float w,float h,string glyph)
    {
        AddHit(id,x,y,w,h);bool hot=_hover==id||_rebinding==id;
        using(var b=new LinearGradientBrush(new RectangleF(x,y,w,h),hot?Color.FromArgb(10,110,145):Color.FromArgb(30,48,68),Color.FromArgb(6,13,24),90f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(53,82,112),hot?2:1)){g.FillRounded(b,new RectangleF(x,y,w,h),5);g.DrawRounded(p,new RectangleF(x,y,w,h),5);}
        using var f=new Font("Segoe UI Symbol",9,FontStyle.Bold);TextRenderer.DrawText(g,glyph,f,Rectangle.Round(new RectangleF(x,y,w,h)),hot?Cyan:Color.FromArgb(100,150,185),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void DrawFace(Graphics g,string id,string glyph,Color colour,float x,float y)
    {
        AddHit(id,x-22,y-22,44,44);bool hot=_hover==id||_rebinding==id;
        float pulse=(float)(Math.Sin(_phase*1.7)+1)*.5f;
        using(var glow=new SolidBrush(Color.FromArgb(hot?65: (int)(12+pulse*12),colour)))g.FillEllipse(glow,x-27,y-27,54,54);
        using(var b=new LinearGradientBrush(new RectangleF(x-19,y-19,38,38),hot?Color.FromArgb(38,75,100):Color.FromArgb(28,44,65),Color.FromArgb(5,11,21),90f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(70,100,133),hot?2.2f:1.2f)){g.FillEllipse(b,x-19,y-19,38,38);g.DrawEllipse(p,x-19,y-19,38,38);}
        using(var p=new Pen(colour,2.3f))g.DrawEllipse(p,x-11,y-11,22,22);
        using var f=new Font("Segoe UI Symbol",15,FontStyle.Bold);TextRenderer.DrawText(g,glyph,f,Rectangle.Round(new RectangleF(x-18,y-18,36,36)),colour,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void DrawStick(Graphics g,string prefix,string click,float x,float y,float dx,float dy)
    {
        AddHit(prefix=="ls"?"l3":"r3",x-12,y-12,24,24);
        int glowAlpha=(int)(8+(Math.Sin(_phase*1.35)+1)*7);
        using(var stickGlow=new SolidBrush(Color.FromArgb(glowAlpha,0,229,255)))g.FillEllipse(stickGlow,x-42,y-42,84,84);
        using(var shadow=new SolidBrush(Color.FromArgb(170,0,0,0)))g.FillEllipse(shadow,x-38,y-34,76,76);
        using(var ring=new LinearGradientBrush(new RectangleF(x-36,y-36,72,72),Color.FromArgb(65,87,112),Color.FromArgb(6,13,23),45f))
        using(var p=new Pen(Color.FromArgb(65,94,128),2)){g.FillEllipse(ring,x-36,y-36,72,72);g.DrawEllipse(p,x-36,y-36,72,72);}
        for(int i=0;i<12;i++){double a=i*Math.PI/6;using var p=new Pen(Color.FromArgb(40,125,155,180),1);g.DrawLine(p,x+(float)Math.Cos(a)*29,y+(float)Math.Sin(a)*29,x+(float)Math.Cos(a)*33,y+(float)Math.Sin(a)*33);}
        float capX=x+dx,capY=y+dy;bool hot=_hover==click||_rebinding==click||_hover?.StartsWith(prefix+"_")==true;
        using(var cap=new LinearGradientBrush(new RectangleF(capX-23,capY-23,46,46),hot?Color.FromArgb(60,105,130):Color.FromArgb(45,61,80),Color.FromArgb(4,10,19),45f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(80,116,148),hot?2:1.2f)){g.FillEllipse(cap,capX-23,capY-23,46,46);g.DrawEllipse(p,capX-23,capY-23,46,46);}
        using(var p=new Pen(Color.FromArgb(35,150,180,200),1)){for(int i=-2;i<=2;i++){g.DrawArc(p,capX-14,capY-14+i*4,28,8,0,180);}}
        using var f=new Font("Segoe UI",7,FontStyle.Bold);TextRenderer.DrawText(g,click,f,Rectangle.Round(new RectangleF(x-20,y+40,40,13)),Color.FromArgb(115,174,205),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void DrawStickDirectionLabel(Graphics g,string prefix,float x,float y)
    {
        using var f=new Font("Segoe UI",7,FontStyle.Bold);
        TextRenderer.DrawText(g,"↑",f,new Rectangle((int)x-7,(int)y-48,14,12),_hover==prefix+"_up"?Cyan:Color.FromArgb(100,155,190),TextFormatFlags.HorizontalCenter);
        TextRenderer.DrawText(g,"↓",f,new Rectangle((int)x-7,(int)y+39,14,12),_hover==prefix+"_down"?Cyan:Color.FromArgb(100,155,190),TextFormatFlags.HorizontalCenter);
        TextRenderer.DrawText(g,"←",f,new Rectangle((int)x-48,(int)y-6,14,12),_hover==prefix+"_left"?Cyan:Color.FromArgb(100,155,190),TextFormatFlags.HorizontalCenter);
        TextRenderer.DrawText(g,"→",f,new Rectangle((int)x+34,(int)y-6,14,12),_hover==prefix+"_right"?Cyan:Color.FromArgb(100,155,190),TextFormatFlags.HorizontalCenter);
    }
    private void DrawPill(Graphics g,string id,string label,float x,float y,float w,float h)
    {
        AddHit(id,x,y,w,h);bool hot=_hover==id||_rebinding==id;
        using(var b=new LinearGradientBrush(new RectangleF(x,y,w,h),Color.FromArgb(34,53,76),Color.FromArgb(5,12,23),90f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(55,84,117),hot?1.8f:1f)){g.FillRounded(b,new RectangleF(x,y,w,h),9);g.DrawRounded(p,new RectangleF(x,y,w,h),9);}
        using var f=new Font("Segoe UI",6.5f,FontStyle.Bold);TextRenderer.DrawText(g,label,f,Rectangle.Round(new RectangleF(x,y,w,h)),hot?Cyan:Color.FromArgb(140,177,204),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void DrawPs(Graphics g)
    {
        AddHit("ps",572,353,56,43);bool hot=_hover=="ps"||_rebinding=="ps";
        using(var glow=new SolidBrush(Color.FromArgb(hot?70:25,0,229,255)))g.FillEllipse(glow,566,347,68,55);
        using(var b=new LinearGradientBrush(new Rectangle(574,355,52,38),Color.FromArgb(48,72,100),Color.FromArgb(6,13,24),90f))
        using(var p=new Pen(hot?Cyan:Color.FromArgb(68,105,143),hot?2:1.3f)){g.FillEllipse(b,574,355,52,38);g.DrawEllipse(p,574,355,52,38);}
        using var f=new Font("Segoe UI",9,FontStyle.Bold);TextRenderer.DrawText(g,"PS",f,new Rectangle(574,355,52,38),hot?Cyan:White,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }

    private void DrawMappingGrid(Graphics g)
    {
        using var h=new Font("Segoe UI",9,FontStyle.Bold);
        using var headerBrush=new SolidBrush(Color.FromArgb(110,190,220));
        g.DrawString("BINDINGS  /  CLICK A ROW OR A CONTROLLER CONTROL TO CAPTURE A KEY",h,headerBrush,28,470);
        const int rows = 9;float x0=28,y0=492,cw=376,rh=41;
        for(int i=0;i<Order.Length;i++)
        {
            string id=Order[i];int col=i/rows,row=i%rows;float x=x0+col*cw,y=y0+row*rh;var r=new RectangleF(x,y,cw-8,rh-5);
            _hits[id]=r;bool hot=_hover==id||_rebinding==id;
            using(var b=new LinearGradientBrush(Rectangle.Round(r),hot?Color.FromArgb(22,68,91):Color.FromArgb(9,17,29),Color.FromArgb(5,11,20),90f))
            using(var p=new Pen(hot?Cyan:Color.FromArgb(42,67,100),hot?1.6f:1f)){g.FillRounded(b,r,6);g.DrawRounded(p,r,6);}
            using var nameFont=new Font("Segoe UI",8.5f,FontStyle.Bold);
            using var keyFont=new Font("Consolas",9,FontStyle.Bold);
            using(var nameBrush=new SolidBrush(hot?Cyan:Color.FromArgb(183,205,225)))
                g.DrawString(Names[id],nameFont,nameBrush,x+10,y+7);
            string key=_rebinding==id?"PRESS A KEY…":DisplayKey(Get(id));
            var badge=new RectangleF(x+cw-125,y+6,108,rh-18);
            using(var bb=new SolidBrush(_rebinding==id?Color.FromArgb(65,65,12):Color.FromArgb(16,32,51)))g.FillRounded(bb,badge,5);
            using(var bp=new Pen(_rebinding==id?Color.FromArgb(255,183,3):hot?Cyan:Border,1))g.DrawRounded(bp,badge,5);
            TextRenderer.DrawText(g,key,keyFont,Rectangle.Round(badge),_rebinding==id?Color.FromArgb(255,203,85):White,TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter|TextFormatFlags.EndEllipsis);
        }
        using var foot=new Font("Segoe UI",8);
        using var footBrush=new SolidBrush(Color.FromArgb(105,150,180));
        g.DrawString("Tip: select a binding, then press any key. Esc cancels. Analog sticks animate when their mapped direction keys are held while this panel has focus.",foot,footBrush,28,866);
    }
    private void DrawRebindOverlay(Graphics g)
    {
        var r=new RectangleF(360,385,480,60);
        using(var b=new SolidBrush(Color.FromArgb(235,4,8,16)))g.FillRounded(b,r,10);
        using(var p=new Pen(Color.FromArgb(255,183,3),2))g.DrawRounded(p,r,10);
        using var f=new Font("Segoe UI",12,FontStyle.Bold);
        TextRenderer.DrawText(g,"PRESS A KEY  ·  ESC TO CANCEL",f,Rectangle.Round(r),Color.FromArgb(255,203,85),TextFormatFlags.HorizontalCenter|TextFormatFlags.VerticalCenter);
    }
    private void AddHit(string id,float x,float y,float w,float h)=>_hits[id]=new RectangleF(x,y,w,h);
}

internal static class ControllerGraphicsExtensions
{
    public static void FillRounded(this Graphics g,Brush b,RectangleF r,float radius)
    { using var p=Rounded(r,radius);g.FillPath(b,p); }
    public static void DrawRounded(this Graphics g,Pen pen,RectangleF r,float radius)
    { using var p=Rounded(r,radius);g.DrawPath(pen,p); }
    private static GraphicsPath Rounded(RectangleF r,float radius)
    {
        float d=radius*2;var p=new GraphicsPath();p.AddArc(r.X,r.Y,d,d,180,90);p.AddArc(r.Right-d,r.Y,d,d,270,90);
        p.AddArc(r.Right-d,r.Bottom-d,d,d,0,90);p.AddArc(r.X,r.Bottom-d,d,d,90,90);p.CloseFigure();return p;
    }
}
