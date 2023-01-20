using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{

    [DataContract]
    public class ProjectData
    {
        [DataMember]
        public string ProjectName { get; set; }
        [DataMember]
        public string ProjectPath { get; set; }
        [DataMember]
        public DateTime Date { get; set; }

        public string FullPath => $"{ProjectPath}{ProjectName}{Project.Extension}";
        public byte[] Icon { get; set; }
        public byte[] Screenshot { get; set; }

    }

    [DataContract]
    internal class ProjectDataList
    {
        [DataMember]
        public List<ProjectData> Projects { get; set; }
    }


    internal class OpenProject
    {
        private static readonly string AppDataPath =
            $@"{Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData)}\LotusEditor\";

        private static readonly string ProjDataPath;

        private static readonly ObservableCollection<ProjectData> _projects = new();

        public static ReadOnlyObservableCollection<ProjectData> Projects { get; }

        static OpenProject()
        {
            try
            {
                if (!Directory.Exists(AppDataPath)) Directory.CreateDirectory(AppDataPath);
                ProjDataPath = $@"{AppDataPath}Projects.xml";
                Projects = new ReadOnlyObservableCollection<ProjectData>(_projects);

                ReadProjectData();
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to read project registry {ProjDataPath}");
                Debug.WriteLine(ex.Message);
                throw;
            }
        }

        public static Project Open(ProjectData projData)
        {
            ReadProjectData();
            var proj = _projects.FirstOrDefault(x => x.FullPath == projData.FullPath);

            if (proj != null)
            {
                proj.Date = DateTime.Now;
            }
            else if(projData != null)
            {
                proj = projData;
                proj.Date = DateTime.Now;
                _projects.Add(proj);
            }

            WriteProjectData();

            return Project.Load(proj?.FullPath).Result;
        }

        public static void AddExistingProject(ProjectData projData)
        {
            ReadProjectData();
            _projects.Add(projData);
            WriteProjectData();
        }

        public static void WriteProjectData()
        {
            var projects = _projects.OrderBy(x => x.Date).ToList();
            Serializer.ToFile(new ProjectDataList() { Projects = projects }, ProjDataPath);
        }

        private static void ReadProjectData()
        {
            if (File.Exists(ProjDataPath))
            {
                var projDataList = Serializer.FromFile<ProjectDataList>(ProjDataPath).Projects;

                var projects = projDataList
                    .OrderByDescending(x => x.Date);
                _projects.Clear();
                foreach (var proj in projects)
                {
                    if (File.Exists(proj.FullPath))
                    {
                        proj.Icon = File.ReadAllBytes($@"{proj.ProjectPath}\.Lotus\icon.png");
                        proj.Screenshot = File.ReadAllBytes($@"{proj.ProjectPath}\.Lotus\screenshot.png");
                        _projects.Add(proj);
                    }
                }

            }
        }
    }
}
