using DiffPlex.DiffBuilder;
using DiffPlex.DiffBuilder.Model;
using System;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace shader_analyzer
{
    public partial class FormDiff : Form
    {
        private string m_FileSideOld;
        private string m_FileSideNew;
        private bool m_AllowUpdates;

        private FileSystemWatcher m_File1Watcher;
        private FileSystemWatcher m_File2Watcher;
        private string m_FileOldContent;
        private string m_FileNewContent;
        private ISideBySideDiffBuilder m_DiffBuilder;

        public FormDiff(string OldFile, string NewFile, bool AllowUpdates)
        {
            InitializeComponent();

            m_FileSideOld = OldFile;
            m_FileSideNew = NewFile;
            m_AllowUpdates = AllowUpdates;
            m_DiffBuilder = new SideBySideDiffBuilder(new DiffPlex.Differ());

            richTextBoxSide1.Font = new Font(FontFamily.GenericMonospace, richTextBoxSide1.Font.Size);
            richTextBoxSide1.WordWrap = false;
            richTextBoxSide1.ScrollBars = RichTextBoxScrollBars.Vertical;
            richTextBoxSide1.VScroll += Side1_VScroll;

            richTextBoxSide2.Font = richTextBoxSide1.Font;
            richTextBoxSide2.WordWrap = richTextBoxSide1.WordWrap;
            richTextBoxSide2.ScrollBars = richTextBoxSide1.ScrollBars;
            richTextBoxSide2.VScroll += Side2_VScroll;
        }

        private void FormDiff_Load(object sender, EventArgs e)
        {
            BuildDiff();

            // Create the file watchers - update if they're modified by an external source
            if (m_AllowUpdates)
            {
                m_File1Watcher = new FileSystemWatcher()
                {
                    Path = Path.GetDirectoryName(m_FileSideOld),
                    Filter = Path.GetFileName(m_FileSideOld),
                    NotifyFilter = NotifyFilters.LastWrite,
                    EnableRaisingEvents = true
                };

                m_File2Watcher = new FileSystemWatcher()
                {
                    Path = Path.GetDirectoryName(m_FileSideNew),
                    Filter = Path.GetFileName(m_FileSideNew),
                    NotifyFilter = NotifyFilters.LastWrite,
                    EnableRaisingEvents = true
                };

                m_File1Watcher.Changed += (s1, e1) => Invoke(new Action(() => BuildDiff()));
                m_File2Watcher.Changed += (s1, e1) => Invoke(new Action(() => BuildDiff()));
            }
        }

        private void FormDiff_Close(object sender, FormClosingEventArgs e)
        {
            m_File1Watcher.Dispose();
            m_File2Watcher.Dispose();
        }

        private void Side1_VScroll(object sender, EventArgs e)
        {
            Win32.SyncVerticalScrollbars(richTextBoxSide2.Handle, richTextBoxSide1.Handle);
        }

        private void Side2_VScroll(object sender, EventArgs e)
        {
            Win32.SyncVerticalScrollbars(richTextBoxSide1.Handle, richTextBoxSide2.Handle);
        }

        private void BuildDiff()
        {
            try
            {
                m_FileOldContent = File.ReadAllText(m_FileSideOld);
                m_FileNewContent = File.ReadAllText(m_FileSideNew);

                BuildDiffView();
            }
            catch(IOException)
            {
                // Drop the call if we can't access the file
            }
        }

        private void BuildDiffView()
        {
            // Show the file paths at the beginning
            richTextBoxSide1.Text = $"   | {m_FileSideOld}" + Environment.NewLine;
            richTextBoxSide2.Text = $"   | {m_FileSideNew}" + Environment.NewLine;

            var diffModel = m_DiffBuilder.BuildDiffModel(m_FileOldContent, m_FileNewContent);

            foreach (DiffPiece line in diffModel.OldText.Lines)
                InsertAndColor(richTextBoxSide1, line);

            foreach (DiffPiece line in diffModel.NewText.Lines)
                InsertAndColor(richTextBoxSide2, line);

            richTextBoxSide1.Select(0, 0);
            richTextBoxSide1.ScrollToCaret();

            richTextBoxSide2.Select(0, 0);
            richTextBoxSide2.ScrollToCaret();
        }

        private void InsertAndColor(RichTextBox TextBox, DiffPiece Line)
        {
            int selectStart = TextBox.Text.Length;
            string lineMarker = string.Format("{0,3:0}| ", Line.Position);
            string inputText = lineMarker + Line.Text;

            // Hack to force background coloring on the entire line
            for (int i = 0; i < 200; i++)
                inputText += " ";

            TextBox.AppendText(inputText + Environment.NewLine);
            TextBox.SelectionStart = selectStart + lineMarker.Length;
            TextBox.SelectionLength = inputText.Length - lineMarker.Length;

            switch (Line.Type)
            {
                case ChangeType.Deleted:
                    TextBox.SelectionBackColor = Color.OrangeRed;
                    break;

                case ChangeType.Inserted:
                    TextBox.SelectionBackColor = Color.LightGreen;
                    break;

                case ChangeType.Modified:
                    TextBox.SelectionBackColor = Color.LightBlue;
                    break;

                default:
                    // Do nothing
                    break;
            }
        }
    }

    public static class Win32
    {
        public enum ScrollBarType : int
        {
            SbHorz = 0,
            SbVert = 1,
            SbCtl = 2,
            SbBoth = 3,
        }

        public enum Message : uint
        {
            WM_VSCROLL = 0x0115,
        }

        public enum ScrollBarCommands : uint
        {
            SB_THUMBPOSITION = 4,
        }

        [DllImport("user32.dll")]
        public static extern int GetScrollPos(IntPtr hWnd, int nBar);

        [DllImport("user32.dll")]
        public static extern int SendMessage(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        public static void SyncVerticalScrollbars(IntPtr TargetHwnd, IntPtr SourceHwnd)
        {
            int nPos = GetScrollPos(SourceHwnd, (int)ScrollBarType.SbVert) << 16;
            uint wParam = (uint)ScrollBarCommands.SB_THUMBPOSITION | (uint)nPos;
            SendMessage(TargetHwnd, (int)Message.WM_VSCROLL, new IntPtr(wParam), IntPtr.Zero);
        }
    }
}
