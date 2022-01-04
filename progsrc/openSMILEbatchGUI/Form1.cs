using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace openSMILEbatchGUI
{
    public partial class Form1 : Form
    {
        List<string> filesToProcessQueue;
        int nextFileIndex = 0;
        int fileSelCount = 0;
        string logfile = "";

        public Form1()
        {
            InitializeComponent();
        }

        private void buildSelFileListFromFolder()
        {
            fileListBox.Items.Clear();

            // iterate through directory
            if (Directory.Exists(selectedFolder.Text))
            {
                string[] files = Directory.GetFiles(selectedFolder.Text, fileExtensionFilter.Text);

                fileListBox.Items.AddRange(files.Select(Path.GetFileName).ToArray());
            }

            selectAllFiles(true);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (selectedFolder.Text != "")
                folderBrowserDialog1.SelectedPath = selectedFolder.Text;
            else
                folderBrowserDialog1.SelectedPath = Directory.GetCurrentDirectory();

            folderBrowserDialog1.RootFolder = Environment.SpecialFolder.MyComputer;

            if (folderBrowserDialog1.ShowDialog() == DialogResult.OK)
            {
                selectedFolder.Text = folderBrowserDialog1.SelectedPath;
                buildSelFileListFromFolder();
            }
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            workDirectory.Text = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "..\\.."));
            if (!refreshConfigFiles())
                workDirectory.Text = "";
        }

        private bool refreshConfigFiles()
        {
            string cd = Path.Combine(workDirectory.Text, "config");

            if (!Directory.Exists(cd))
                return false;                

            string[] files = Directory.GetFiles(cd, "*.conf");
            string defaultConfig = "";

            foreach (string f in files)
            {
                string fname = Path.GetFileName(f);

                configurations.Items.Add(fname);

                if (fname == "GenevaExtended.conf")
                    defaultConfig = fname;
                else if (defaultConfig == "" && fname == "IS09_emotion.conf")
                    defaultConfig = fname;
            }

            // default: Geneva Extended, if available, else IS09 emotion
            configurations.Text = defaultConfig;

            return true;
        }

        private void button6_Click(object sender, EventArgs e)
        {
            selectedFolder.Text = Path.GetFullPath(selectedFolder.Text);
            buildSelFileListFromFolder();
        }

        private void selectAllFiles(bool state = true)
        {
            for (int i = 0; i < fileListBox.Items.Count; i++)
                fileListBox.SetItemChecked(i, state);
        }

        private void button3_Click(object sender, EventArgs e)
        {
            selectAllFiles(true);    
        }

        private void button4_Click_1(object sender, EventArgs e)
        {
            selectAllFiles(false);    
        }

        private void initProgress(int N)
        {
            progressBar1.Maximum = N;
            progressBar1.Minimum = 0;
            progressBar1.Step = 1;
            progressBar1.Value = 0;
        }

        private void logProgress(int i, int N)
        {
            progressBar1.Value = i;
            progressLabel.Text = string.Format("Processing file {0} of {1}...", i, N);
        }

        private void logOutput(string line)
        {
            logBox.AppendText(line + "\r\n");
        }

        private int runSmileBinary(string workpath, string binary, string options)
        {
            try
            {
                // run opensmile
                Process p = new Process();

                p.EnableRaisingEvents = true;
                p.StartInfo.FileName = binary;
                p.StartInfo.Arguments = options;
                p.StartInfo.WorkingDirectory = workpath;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = true;
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.RedirectStandardError = true;
                p.ErrorDataReceived += new DataReceivedEventHandler(p_ErrorDataReceived);
                p.Exited += new EventHandler(p_Exited);

                // delete old log file
                if (File.Exists(logfile))
                    File.Delete(logfile);

                if (p.Start())
                    return p.Id;
                else
                    return 0;
            }
            catch (Win32Exception e)
            {
                MessageBox.Show(string.Format("Could not run {0} with arguments {1}. {2}", binary, options, e.Message), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 0;
            }
        }

        private void p_ErrorDataReceived(object sendingProcess, DataReceivedEventArgs errLine)
        {
            // Write the error text to the file if there is something 
            // to write and an error file has been specified. 

            if (!string.IsNullOrEmpty(errLine.Data))
            {
                Invoke(new MethodInvoker(delegate
                {
                    logOutput(errLine.Data);
                }));
            }
        }

        // a smile process has finished
        void p_Exited(object sender, EventArgs e)
        {
            // get exit code from process object
            Process p = (Process)sender;
            // load log file:
            string txt = File.ReadAllText(logfile);
            Invoke(new MethodInvoker(delegate
            {
                logOutput(txt + "\n");
            }));
            if (p.ExitCode != 0)
            {
                // error running smile...
                Invoke(new MethodInvoker(delegate
                {
                    progressLabel.Text = "An error occurred, see log for details.";
                    prepareUiAfterAnalysisRun();
                }));
                // exit the loop

                /*int idx = ad.getIndex();
                listView1.Invoke(new MethodInvoker(delegate
                {
                    listView1.Items[idx].Checked = false;
                    listView1.Items[idx].SubItems[2].Text = "error";
                    listView1.Items[idx].ForeColor = Color.Gray;
                }
                ));*/
            }
            else
            {
                // run next file in loop
                Invoke(new MethodInvoker(delegate
                {
                    processNextFile();
                }));
            }
            p.Dispose();
        }

        private string getLabels(string filename)
        {
            if (labels == null)
                return "";

            string basename = Path.GetFileNameWithoutExtension(filename);
            string opts = "";
            string[] vals = labels[basename];

            if (vals != null)
            {
                for (int i = 1; i < vals.Length; i++)
                    opts += "-" + classnames[i] + " " + vals[i] + " ";
                //MessageBox.Show(opts);
            }

            return opts;
        }
        
        private void processNextFile()
        {
            bool next = true;
            while (next)
            {
                logProgress(nextFileIndex, fileSelCount);
                if (nextFileIndex < fileSelCount)
                {
                    string f = filesToProcessQueue[nextFileIndex];
                    nextFileIndex++;
                    Invoke(new MethodInvoker(delegate
                    {
                        // get labels
                        string extraoptions = getLabels(f);
                        if (labelFile.Text != "" && extraoptions == "")
                        {
                            logOutput("Skipping file '" + f + "'. Not in label list.\n");
                        }
                        else
                        {
                            logOutput("Processing file '" + f + "'.\n");
                            next = false;
                            // run openSMILE
                            runOpenSMILE(f, extraoptions);
                        }
                    }
                    ));
                }
                else
                {
                    Invoke(new MethodInvoker(delegate
                    {
                        progressLabel.Text = "Done.";
                        prepareUiAfterAnalysisRun();
                    }));
                    next = false;
                }
            }
        }

        Dictionary<string, string[]> labels;
        string[] classnames;

        private void runOpenSMILE(string filename, string extraoptions="")
        {
            string binary = Path.GetFullPath(Path.Combine(workDirectory.Text, "bin", "Win32", "SMILExtract_Release.exe"));
            logfile = Path.Combine(Path.GetTempPath(), "smile.log");
            string options = "-C \"" + Path.GetFullPath(Path.Combine(workDirectory.Text, "config", configurations.Text)) + "\" ";
            options += "-logfile \"" + logfile + "\" ";
            string wavFilename = Path.GetFullPath(filename);
            options += "-I \"" + wavFilename + "\" ";
            options += "-instname \"" + Path.GetFileName(wavFilename) + "\" ";

            if (haveArffOut.Checked)
            {
                string arffOutFile = Path.GetFullPath(arffOutName.Text);
                options += "-O \"" + arffOutFile + "\" ";
                if (arffAppend.Checked)
                    options += "-appendarff 1 ";  // note: this is the default
                else
                    options += "-appendarff 0 ";
            }
            else
                options += "-O ? ";

            if (haveCsvOut.Checked)
            {
                string csvOutFile = Path.GetFullPath(csvOutName.Text);
                options += "-csvoutput \"" + csvOutFile + "\" ";
                if (csvAppend.Checked)
                    options += "-appendcsv 1 ";
                else
                    options += "-appendcsv 0 ";
            }
            else
                options += "-csvoutput ? ";

            if (haveLldCsvOut.Checked)
            {
                string lldCsvOutFile = Path.GetFullPath(lldCsvOutName.Text);
                options += "-lldcsvoutput \"" + lldCsvOutFile + "\" ";
                if (lldCsvAppend.Checked)
                    options += "-appendcsvlld 1 ";
                else
                    options += "-appendcsvlld 0 ";
            }
            else
                options += "-lldcsvoutput ? ";

            options += "-l 2 ";
            options += extraoptions + " ";
            logBox.AppendText("Running command: '" + binary + " " + options + "'\r\n");

            if (runSmileBinary(Path.GetFullPath(workDirectory.Text), binary, options) == 0)
                prepareUiAfterAnalysisRun();
        }

        private void prepareUiForAnalysisRun()
        {
            // disable all controls that should not be changed during the analysis run
            groupBox1.Enabled = false;
            groupBox3.Enabled = false;
            button1.Enabled = false;
            button5.Enabled = true;            
        }

        private void prepareUiAfterAnalysisRun()
        {
            groupBox1.Enabled = true;
            groupBox3.Enabled = true;
            button1.Enabled = true;
            button5.Enabled = false;
        }

        private void runAnalysisLoop()
        {
            fileSelCount = fileListBox.CheckedItems.Count;

            prepareUiForAnalysisRun();
            initProgress(fileSelCount);
            // TODO: thread this!

            filesToProcessQueue = new List<string>();

            foreach (string file in fileListBox.CheckedItems.OfType<string>())
                filesToProcessQueue.Add(Path.Combine(selectedFolder.Text, file));

            // in append mode, delete existing files
            if (haveArffOut.Checked && arffAppend.Checked && File.Exists(arffOutName.Text))
            {
                DialogResult dlrs = MessageBox.Show("The ARFF output file already exists. The existing file will be deleted now. Proceed?", "Overwrite files?", MessageBoxButtons.YesNo);
                if (dlrs == DialogResult.Yes)
                {
                    File.Delete(arffOutName.Text);
                }
                else
                {
                    prepareUiAfterAnalysisRun();
                    return;
                }
            }

            if (haveCsvOut.Checked && csvAppend.Checked && File.Exists(csvOutName.Text))
            {
                DialogResult dlrs = MessageBox.Show("The CSV output file already exists. The existing file will be deleted now. Proceed?", "Overwrite files?", MessageBoxButtons.YesNo);
                if (dlrs == DialogResult.Yes)
                {
                    File.Delete(csvOutName.Text);
                }
                else
                {
                    prepareUiAfterAnalysisRun();
                    return;
                }
            }

            if (haveLldCsvOut.Checked && lldCsvAppend.Checked && File.Exists(lldCsvOutName.Text))
            {
                DialogResult dlrs = MessageBox.Show("The LLD CSV output file already exists. The existing file will be deleted now. Proceed?", "Overwrite files?", MessageBoxButtons.YesNo);
                if (dlrs == DialogResult.Yes)
                {
                    File.Delete(lldCsvOutName.Text);
                }
                else
                {
                    prepareUiAfterAnalysisRun();
                    return;
                }
            }

            nextFileIndex = 0;
            processNextFile();  // start with the first
        }

        private bool loadLabelList()
        {
            if (labelFile.Text == "")
                return true;

            if (!File.Exists(labelFile.Text))
            {
                MessageBox.Show("ERROR: Label file not found! Please leave field empty if you do not have a label file.");
                return false;
            }

            string[] labelLines = File.ReadAllLines(labelFile.Text);
            string[] labelVals = labelLines[0].Split(';');
            classnames = new[] { "Filename" }.Concat(labelVals.Skip(1)).ToArray();

            labels = new Dictionary<string, string[]>();
            for (int i = 1; i < labelLines.Length; i++) 
            {
                string[] labelValues = labelLines[i].Split(';');
                string filename = Path.GetFileNameWithoutExtension(labelValues[0]);
                labels.Add(filename, labelValues);
            }
            return true;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (workDirectory.Text == "")
            {
                MessageBox.Show("Please specify the path to a copy of the openSMILE distribution first.", "openSMILE directory path not set", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            else if (configurations.Text == "")
            {
                MessageBox.Show("Please refresh the list of openSMILE configurations first and select one of the configuration files.", "No configuration selected", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            else if (fileListBox.Items.Count == 0)
            {
                MessageBox.Show("Select a folder with the input audio files first and click on \"2. Refresh Audio Files\".", "No input files selected", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            else if (fileListBox.CheckedItems.Count == 0)
            {
                MessageBox.Show("Check at least one of the input files to process.", "No input files selected", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (haveArffOut.Checked && arffOutName.Text == "")
            {
                MessageBox.Show("You have not specified a path where to save the ARFF output.", "No ARFF output path specified", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            else if (haveCsvOut.Checked && csvOutName.Text == "")
            {
                MessageBox.Show("You have not specified a path where to save the CSV output.", "No CSV output path specified", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            else if (haveLldCsvOut.Checked && lldCsvOutName.Text == "")
            {
                MessageBox.Show("You have not specified a path where to save the LLD CSV output.", "No LLD CSV output path specified", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            if (loadLabelList())
                runAnalysisLoop();
        }

        private void button7_Click(object sender, EventArgs e)
        {
            if (saveFileDialogArff.ShowDialog() == DialogResult.OK) 
            {
                arffOutName.Text = saveFileDialogArff.FileName;
                haveArffOut.Checked = true;
            }
        }

        private void button8_Click(object sender, EventArgs e)
        {
            if (saveFileDialogLldCsv.ShowDialog() == DialogResult.OK) 
            {
                lldCsvOutName.Text = saveFileDialogLldCsv.FileName;
                haveLldCsvOut.Checked = true;
            }
        }

        private void button9_Click(object sender, EventArgs e)
        {
            if (saveFileDialogCsv.ShowDialog() == DialogResult.OK) 
            {
                csvOutName.Text = saveFileDialogCsv.FileName;
                haveCsvOut.Checked = true;
            }
        }

        private void button10_Click(object sender, EventArgs e)
        {
            folderBrowserDialog2.RootFolder = Environment.SpecialFolder.MyComputer;
            folderBrowserDialog2.SelectedPath = workDirectory.Text;

            if (folderBrowserDialog2.ShowDialog() != DialogResult.OK)
                return;

            workDirectory.Text = folderBrowserDialog2.SelectedPath;

            if (!refreshConfigFiles())
            {
                MessageBox.Show("Could not find directory with openSMILE configuration files under the specified path.", "Invalid path", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (Directory.Exists(workDirectory.Text))
                Directory.CreateDirectory(Path.Combine(workDirectory.Text, "data"));
        }

        private void button5_Click(object sender, EventArgs e)
        {
            filesToProcessQueue.Clear();
        }

        private void button11_Click_1(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
                labelFile.Text = openFileDialog1.FileName;
        }

        private void button12_Click(object sender, EventArgs e)
        {
            StringBuilder message = new StringBuilder();

            message.AppendLine("In order to specify labels for each wave file which will be added to the ARFF output file " +
                "(for classification with the WEKA data mining toolkit), use a semicolon-separated text file of the following format:");
            message.AppendLine();
            message.AppendLine("Filename; LabelA_name; LabelB_name;...; LabelZ_name");
            message.AppendLine("mywav1.xxx; angry; neutral;...; female");
            message.AppendLine("mywav2.xxx; happy; neutral;...; female");
            message.AppendLine();
            message.AppendLine("The first line is a header line which lists the names of the labels, as they should appear in the ARFF header. " +
                "The following lines contain the labels for each file. The filename thereby must be unique (no path is included), and the extension is ignored. " +
                "All columns/entries are to be separated by a semicolon (;) with no spaces inbetween!");

            MessageBox.Show(message.ToString(), "Label file format", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void button13_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(workDirectory.Text) || !refreshConfigFiles())
            {
                MessageBox.Show("Could not find directory with openSMILE configuration files under the specified path.", "Invalid path", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            workDirectory.Text = Path.GetFullPath(workDirectory.Text);

            if (arffOutName.Text == "")
                arffOutName.Text = Path.Combine(workDirectory.Text, "data", "output.arff");

            if (csvOutName.Text == "")
                csvOutName.Text = Path.Combine(workDirectory.Text, "data", "output.csv");

            if (lldCsvOutName.Text == "")
                lldCsvOutName.Text = Path.Combine(workDirectory.Text, "data", "output_lld.csv");
        }

        private void HaveArffOut_CheckStateChanged(object sender, EventArgs e)
        {
            arffOutName.Enabled = button7.Enabled = arffAppend.Enabled = haveArffOut.Checked;
            csvOutName.Enabled = button9.Enabled = csvAppend.Enabled = haveCsvOut.Checked;
            lldCsvOutName.Enabled = button8.Enabled = lldCsvAppend.Enabled = haveLldCsvOut.Checked;
        }
    }
}
