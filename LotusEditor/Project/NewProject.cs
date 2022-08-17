using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.Utility;

namespace LotusEditor.Project
{
    [DataContract]
    public class ProjectTemplate
    {
        [DataMember]
        public string? ProjectType { get; set; }
        [DataMember]
        public string? ProjectFile { get; set; }
        [DataMember]
        public List<string>? Folders { get; set; }

        public byte[]? Icon { get; set; }
        public byte[]? Screenshot { get; set; }

        public string? IconFilePath { get; set; }
        public string? ScreenshotFilePath { get; set; }
        public string? ProjectFilePath { get; set; }
    }

    class NewProject : ViewModelBase
    {
        // #TODO: Path will change on release/distribution. Need to get from install location
        private readonly string _templatePath = @"..\..\LotusEditor\ProjectTemplates";

        private string _name = "New Project";
        private string _path = $@"{Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)}\LotusProjects\";

        private ObservableCollection<ProjectTemplate> _projectTemplates = new ObservableCollection<ProjectTemplate>();

        public ReadOnlyObservableCollection<ProjectTemplate> ProjectTemplates { get; }

        private bool _isValid;

        private string _errorMessage;

        public NewProject()
        {
            ProjectTemplates = new ReadOnlyObservableCollection<ProjectTemplate>(_projectTemplates);
            try
            {
                var templates = Directory.GetFiles(_templatePath, "template.xml", SearchOption.AllDirectories);
                Debug.Assert(templates.Any());

                foreach (var file in templates)
                {
                    var template = Serializer.FromFile<ProjectTemplate>(file);
                    template.IconFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file)!, "icon.png"));
                    template.Icon = File.ReadAllBytes(template.IconFilePath);
                    template.ScreenshotFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file)!, "screenshot.png"));
                    template.Screenshot = File.ReadAllBytes(template.ScreenshotFilePath);
                    template.ProjectFilePath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(file)!, template.ProjectFile!));

                    _projectTemplates.Add(template);
                }
                ValidateProjectPath();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                // #TODO: Log console msgs
            }
        }

        private bool ValidateProjectPath()
        {
            var path = ProjectPath;
            if (!Path.EndsInDirectorySeparator(path))
            {
                path += @"\";
            }

            path += $@"{ProjectName}\";

            IsValid = true;

            if (string.IsNullOrWhiteSpace(ProjectName.Trim()))
            {
                ErrorMessage = "You must supply a project name";
                IsValid = false;
            }

            if (ProjectName.IndexOfAny(Path.GetInvalidFileNameChars()) != -1)
            {
                ErrorMessage = "Invalid character(s) in project name!";
                IsValid = false;
            }

            if (string.IsNullOrWhiteSpace(ProjectPath.Trim()))
            {
                ErrorMessage = "Select a valid project path";
                IsValid = false;
            }

            if (ProjectPath.IndexOfAny(Path.GetInvalidPathChars()) != -1)
            {
                ErrorMessage = "Invalid character(s) in project path!";
                IsValid = false;
            }

            if (Directory.Exists(path) && Directory.EnumerateFileSystemEntries(path).Any())
            {
                ErrorMessage = "The provided path already exists and is not empty";
                IsValid = false;
            }

            if (IsValid) ErrorMessage = string.Empty;

            return IsValid;
        }

        public string ProjectName
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectName));
                }
            }
        }

        public string ProjectPath
        {
            get => _path;
            set
            {
                if (_path != value)
                {
                    _path = value;
                    ValidateProjectPath();
                    OnPropertyChanged(nameof(ProjectPath));
                }
            }
        }

        public bool IsValid
        {
            get => _isValid;
            set
            {
                if (_isValid != value)
                {
                    _isValid = value;
                    OnPropertyChanged(nameof(IsValid));
                }
            }
        }

        public string ErrorMessage
        {
            get => _errorMessage;
            set
            {
                if (_errorMessage != value)
                {
                    _errorMessage = value;
                    OnPropertyChanged(nameof(ErrorMessage));
                }
            }
        }

    }

}
