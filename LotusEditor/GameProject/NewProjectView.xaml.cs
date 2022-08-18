using System;
using System.Windows;
using System.Windows.Forms;
using UserControl = System.Windows.Controls.UserControl;

namespace LotusEditor.GameProject
{
    /// <summary>
    /// Interaction logic for NewProjectView.xaml
    /// </summary>
    public partial class NewProjectView : UserControl
    {
        public NewProjectView()
        {
            InitializeComponent();
        }

        private void Create_ButtonBase_OnClick(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as NewProject;
            var projPath = vm.CreateProject(templateListBox.SelectedItem as ProjectTemplate);
            bool dialogResult = false;
            var win = Window.GetWindow(this);

            if (!string.IsNullOrEmpty(projPath))
            {
                dialogResult = true;
                var proj = OpenProject.Open(new ProjectData() { ProjectName = vm.ProjectName, ProjectPath = projPath });
                win.DataContext = proj;
            }

            win.DialogResult = dialogResult;
            win.Close();
        }

        private void Browse_Button_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new FolderBrowserDialog();
            dialog.InitialDirectory = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\LotusProjects\";
            dialog.ShowNewFolderButton = true;
            dialog.Description = "Select the location to create the Lotus Project folder at";
            dialog.UseDescriptionForTitle = true;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                var vm = DataContext as NewProject;
                vm.ProjectPath = dialog.SelectedPath;
            }
        }
    }
}
