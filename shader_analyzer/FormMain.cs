using System;
using System.IO;
using System.Windows.Forms;

namespace shader_analyzer
{
    public partial class FormMain : Form
    {
        private string[] m_FileList;

        public FormMain()
        {
            InitializeComponent();
        }

        private void FormMain_Load(object sender, EventArgs e)
        {
            listViewShaders.Columns.Add(new ColumnHeader
            {
                Text = "Shader",
                Width = 300
            });

            listViewShaders.Columns.Add(new ColumnHeader
            {
                Text = "Symbolic",
                Width = 100
            });

            listViewShaders.Columns.Add(new ColumnHeader
            {
                Text = "Whitelisted",
                Width = 100
            });

            listViewShaders.View = View.Details;
            listViewShaders.FullRowSelect = true;
            listViewShaders.MultiSelect = false;
            PopulateListView();
        }

        private void listViewShaders_DoubleClick(object sender, EventArgs e)
        {
            // Bring up a diff view with the selected file
            string selectedFile = listViewShaders.SelectedItems[0].Text;

            FormDiff differ = new FormDiff($"{Program.ShaderDiffDirectory}\\{selectedFile}-old.txt", $"{Program.ShaderDiffDirectory}\\{selectedFile}-new.txt", true);
            differ.Show();
        }

        private void buttonRun_Click(object sender, EventArgs e)
        {
            buttonRun.Enabled = false;
            ClearListView();

            ShaderAnalyzer.DoStuff(() =>
            {
                Invoke(new Action(() => { PopulateListView(); buttonRun.Enabled = true; }));
            });
        }

        private void buttonClearList_Click(object sender, EventArgs e)
        {
            ClearListView();
        }

        private void buttonClearLog_Click(object sender, EventArgs e)
        {
            richTextBoxLog.Text = "";
            richTextBoxLog.SelectionStart = 0;
        }

        private void textBoxFilter_TextChanged(object sender, EventArgs e)
        {
            listViewShaders.Items.Clear();
            PopulateListView();
        }

        private void buttonDecompile_Click(object sender, EventArgs e)
        {
            buttonDecompile.Enabled = false;

            ShaderAnalyzer.DecompileShaders(() =>
            {
                Invoke(new Action(() => { buttonDecompile.Enabled = true; }));
            });
        }

        private void PopulateListView()
        {
            m_FileList = Directory.GetFiles(Program.ShaderDiffDirectory, "*.*");

            //
            // ListView layout:
            //
            // "Shader", "Symbolic", "Whitelisted"
            //
            listViewShaders.BeginUpdate();

            foreach (string file in m_FileList)
            {
                // Completely skip any file with "-new" in the name to eliminate duplicate listings
                if (file.Contains("-new"))
                    continue;

                if (textBoxFilter.Text.Length > 0 && !file.Contains(textBoxFilter.Text))
                    continue;

                string fixedFile = Path.GetFileNameWithoutExtension(file).Replace("-old", "");
                ListViewItem item = new ListViewItem(fixedFile);

                if (fixedFile.Contains("-symbolic"))
                    item.SubItems.Add("Yes");
                else
                    item.SubItems.Add("No");

                item.SubItems.Add("No");
                listViewShaders.Items.Add(item);
            }

            listViewShaders.EndUpdate();
        }

        private void ClearListView()
        {
            if (Directory.Exists(Program.ShaderDiffDirectory))
                Directory.Delete(Program.ShaderDiffDirectory, true);

            Directory.CreateDirectory(Program.ShaderDiffDirectory);
            listViewShaders.Items.Clear();
        }

        public void Log(string Format, params object[] Parameters)
        {
            richTextBoxLog.Text += string.Format(Format, Parameters);

            richTextBoxLog.SelectionStart = richTextBoxLog.Text.Length;
            richTextBoxLog.ScrollToCaret();
        }

        public void LogLine(string Format, params object[] Parameters)
        {
            Log(Format + Environment.NewLine, Parameters);
        }
    }
}
