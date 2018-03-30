using System;
using System.IO;
using System.Windows.Forms;

namespace shader_analyzer
{
    public partial class FormMain : Form
    {
        public FormMain()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
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

            FormDiff differ = new FormDiff($"C:\\Diffs\\{selectedFile}-new.txt", $"C:\\diffs\\{selectedFile}-old.txt", true);
            differ.Show();
        }

        private void buttonRun_Click(object sender, EventArgs e)
        {
            ShaderAnalyzer.DoStuff();
        }

        private void PopulateListView()
        {
            listViewShaders.Items.Clear();
            string[] fileList = Directory.GetFiles("C:\\diffs\\", "*.*");

            //
            // ListView layout:
            //
            // "Shader", "Symbolic", "Whitelisted"
            //
            foreach (string file in fileList)
            {
                // Completely skip any file with "-new" in the name to eliminate duplicate listings
                if (file.Contains("-new"))
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
