using System;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Forms;
using System.Windows.Input;
using UserControl = System.Windows.Controls.UserControl;

namespace LotusEditor.GameProject
{
    /// <summary>
    /// Interaction logic for OpenProjectView.xaml
    /// </summary>
    public partial class OpenProjectView : UserControl
    {
        public OpenProjectView()
        {
            InitializeComponent();
            Loaded += (s, e) =>
            {
                var item =
                    projectsListBox.ItemContainerGenerator.ContainerFromIndex(projectsListBox.SelectedIndex) as
                        ListBoxItem;
                item?.Focus();

            };
        }

        private void Open_Button_Click(object sender, RoutedEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OnListBoxItem_DoubleClick(object sender, MouseButtonEventArgs e)
        {
            OpenSelectedProject();
        }

        private void OpenSelectedProject()
        {
            var project = OpenProject.Open(projectsListBox.SelectedItem as ProjectData);
            bool dialogResult = false;
            var win = Window.GetWindow(this);

            if (project != null)
            {
                dialogResult = true;
                win.DataContext = project;
            }

            win.DialogResult = dialogResult;
            win.Close();
        }


        private void AddExisting_Button_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Filter = "Lotus Files|*.lproj";
            dialog.InitialDirectory =
                $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\LotusProjects\";
            dialog.Title = "Please select a Lotus project file to add";
            DialogResult res = dialog.ShowDialog();
            if (res == DialogResult.OK)
            {
                var f = dialog.FileName;
                var projPath = Path.GetDirectoryName(f);
                var projName = Path.GetFileNameWithoutExtension(f);

                var projData = new ProjectData
                {
                    ProjectName = projName, ProjectPath = $@"{projPath}\",
                    Date = DateTime.Now
                };
                projData.Icon = File.ReadAllBytes($@"{projData.ProjectPath}\.Lotus\icon.png");
                projData.Screenshot = File.ReadAllBytes($@"{projData.ProjectPath}\.Lotus\screenshot.png");
                OpenProject.AddExistingProject(projData);
            }
        }
    }
}
