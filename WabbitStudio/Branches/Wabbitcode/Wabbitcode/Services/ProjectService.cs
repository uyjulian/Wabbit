﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Forms;
using Revsoft.Wabbitcode.Classes;
using Revsoft.Wabbitcode.Docking_Windows;
using System.Collections;
using Revsoft.Wabbitcode.Properties;
using System.Threading;
using Revsoft.Wabbitcode.Services.Parser;
using Revsoft.Wabbitcode.Services.Project;

namespace Revsoft.Wabbitcode.Services
{
	public static class ProjectService
	{
		private static ProjectClass project;
		public static ProjectClass Project
		{
			get { return project; }
			set { project = value; }
		}

		public static string ProjectDirectory
		{
			get { return project.ProjectDirectory; }
		}

		private static List<ParserInformation> parseInfo = new List<ParserInformation>();
		public static IList<ParserInformation> ParseInfo
		{
			get { return parseInfo; }
		}

		public static string ProjectFile
		{
			get { return project.ProjectFile; }
		}

		public static string ProjectName
		{
			get
			{
				if (project == null)
					return null;
				return project.ProjectName;
			}
			set { project.ProjectName = value; }
		}

		private static bool isInternal = true;
		public static bool IsInternal
		{
			get { return isInternal; }
			set { isInternal = value; }
		}

		public static List<string> IncludeDirs {
			get { return project.IncludeDir; }
			set { project.IncludeDir = value; }
		}

		public static ProjectFolder MainFolder
		{
			get
			{
				if (project == null)
					return null;
				return project.MainFolder;
			}
			set { project.MainFolder = value; }
		}

		static FileSystemWatcher projectWatcher;
		public static FileSystemWatcher ProjectWatcher {
			get { return projectWatcher; }
		}

		public static bool OpenProject(string fileName)
		{
            if (!OpenProject(fileName, false))
                return false;
			DockingService.MainForm.UpdateProjectMenu(true);
            DockingService.ProjectViewer.BuildProjTree();
			return true;
		}

		public static bool OpenProject(string fileName, bool closeFiles)
		{
			if (closeFiles)
				foreach (Form mdiChild in DockingService.Documents)
					mdiChild.Close();
            project = new ProjectClass(fileName);
            project.OpenProject(fileName);
            

            if (!InitWatcher(project.ProjectDirectory))
                return false;
            if (closeFiles)
            {
                DockingService.ShowDockPanel(DockingService.ProjectViewer);
                DockingService.ProjectViewer.BuildProjTree();
            }
            DockingService.DirectoryViewer.buildDirectoryTree(Directory.GetFiles(ProjectDirectory));
			DockingService.MainForm.UpdateProjectMenu(true);

			//ThreadStart threadStart = new ThreadStart(GetIncludeDirectories);
            ThreadPool.QueueUserWorkItem(ParseFiles);

			if (Settings.Default.startupProject != fileName)
				if (MessageBox.Show("Would you like to make this your default project?", "Startup Project",
									MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
					Settings.Default.startupProject = fileName;
            return true;
		}

		private static void ParseFiles(object data)
		{
            parseInfo.Clear();
			ParseFiles(MainFolder);
		}

		private static void ParseFiles(ProjectFolder folder)
		{
			foreach (ProjectFolder subFolder in folder.Folders)
				ParseFiles(subFolder);
            ProjectFile[] filesToParse = new ProjectFile[folder.Files.Count];
            folder.Files.CopyTo(filesToParse, 0);
			foreach (ProjectFile file in filesToParse)
				ParserService.ParseFile(file.FileFullPath);
		}

		private static string GetProjectIncludeDirectories(IEnumerable<string> directories)
		{
			string includeDir = "";
			foreach (string directory in directories)
			{
				GetProjectIncludeDirectories(Directory.GetDirectories(directory));
				includeDir += directory + "\n";
			}
			return includeDir;
		}

		private static bool InitWatcher(string location)
		{
#if !DEBUG
            try
            {
#endif
                projectWatcher = new FileSystemWatcher(location);
                projectWatcher.Changed += new FileSystemEventHandler(projectWatcher_Changed);
                projectWatcher.Deleted += new FileSystemEventHandler(projectWatcher_Deleted);
                projectWatcher.Renamed += new RenamedEventHandler(projectWatcher_Renamed);
                projectWatcher.Created += new FileSystemEventHandler(projectWatcher_Created);
                projectWatcher.EnableRaisingEvents = true;
                projectWatcher.IncludeSubdirectories = true;
                projectWatcher.Path = location;
#if !DEBUG
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error in InitWatcher\n" + ex.ToString());
                return false;
            }
#endif
                return true;
		}

		static void projectWatcher_Created(object sender, FileSystemEventArgs e)
		{

		}

		static void projectWatcher_Renamed(object sender, RenamedEventArgs e)
		{
			if (e.OldFullPath == ProjectDirectory)
			{
				if (MessageBox.Show("Project Folder was renamed, would you like to rename the project?", "Rename project",
					MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
					ProjectName = Path.GetFileNameWithoutExtension(e.FullPath);
				return;
			}
		}

		static void projectWatcher_Deleted(object sender, FileSystemEventArgs e)
		{
			//throw new NotImplementedException();
		}

		delegate void FileChangedDelegate(NewEditor doc, string fileName);
		static void projectWatcher_Changed(object sender, FileSystemEventArgs e)
		{
			switch (e.ChangeType)
			{
				case WatcherChangeTypes.Changed:
					if (!DocumentService.InternalSave && !string.IsNullOrEmpty(Path.GetExtension(e.FullPath)))
					{
						foreach (NewEditor doc in DockingService.Documents)
						{
							if (string.Equals(doc.FileName, e.FullPath, StringComparison.OrdinalIgnoreCase))
							{
								FileChangedDelegate fileChanged = UpdateFileChanged;
								DockingService.MainForm.Invoke(fileChanged, new object[] { doc, e.FullPath });
								break;
							}
						}
					}
					break;
			}
		}

		internal static void UpdateFileChanged(NewEditor doc, string fileName)
		{
			projectWatcher.EnableRaisingEvents = false;
			DialogResult result = MessageBox.Show(fileName + " modified outside the editor.\nLoad changes?", "File modified", MessageBoxButtons.YesNo);
			if (result == System.Windows.Forms.DialogResult.Yes)
				DocumentService.OpenDocument(doc, fileName);
			projectWatcher.EnableRaisingEvents = true;
		}

		internal static void CreateInternalProject()
		{
			project = new ProjectClass();
			isInternal = true;
			DockingService.MainForm.UpdateProjectMenu(false);
			DockingService.ProjectViewer.BuildProjTree();
		}

		internal static void CreateNewProject(string projectFile, string projectName)
		{
#if !DEBUG
            try
            {
#endif
                project = new ProjectClass();
                project.CreateNewProject(projectFile, projectName);
                isInternal = false;
                DockingService.ShowDockPanel(DockingService.ProjectViewer);
#if !DEBUG
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to create new project file\n" + ex.ToString());
            }
#endif
        }

		internal static void DeleteFolder(ProjectFolder parentDir, ProjectFolder dir)
		{
			project.DeleteFolder(parentDir, dir);
            project.NeedsSave = true;
		}

        internal static void DeleteFile(string fullPath)
        {
            ProjectFile file = project.FindFile(fullPath);
            DeleteFile(file.Folder, file);
            project.NeedsSave = true;
        }

        internal static void DeleteFile(ProjectFolder parentDir, ProjectFile file)
        {
            ParserService.RemoveParseData(file.FileFullPath);
            project.DeleteFile(parentDir, file);
            project.NeedsSave = true;
        }

		internal static void SaveProject()
		{
#if !DEBUG
            try
            {

#endif
                project.BuildXMLFile();
                DockingService.ProjectViewer.BuildProjTree();
                project.NeedsSave = false;
#if !DEBUG
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error saving project\n" + ex.ToString());
            }
#endif
		}

		internal static ProjectFolder AddFolder(string dirName, ProjectFolder parentDir)
		{
            return project.AddFolder(dirName, parentDir);
		}

		internal static ProjectFile AddFile(ProjectFolder parent, string fullPath)
		{
            return project.AddFile(parent, fullPath);
		}

		internal static void CloseProject()
		{
            DialogResult result = DialogResult.No;
            if (Project.NeedsSave && !Settings.Default.autoSaveProject)
                result = MessageBox.Show("Would you like to save your changes to the project file?", "Save project?", MessageBoxButtons.YesNo, MessageBoxIcon.None);
            if (result == DialogResult.Yes || Settings.Default.autoSaveProject)
                SaveProject();
            isInternal = true;
		}

		internal static ParserInformation GetParseInfo()
		{
			return GetParseInfo(DocumentService.ActiveFileName);
		}

		internal static ParserInformation GetParseInfo(string file)
		{
            lock (parseInfo)
            {
                for (int i = 0; i < parseInfo.Count; i++)
                {
                    var info = parseInfo[i];
                    if (string.Equals(info.SourceFile, file))
                        return info;
                }
            }
			return null;
		}

		internal static bool ContainsFile(string file)
		{
            return file == null ? false :project.ContainsFile(file);
		}

		public static IList<BuildConfig> BuildConfigs
		{
			get { return project.BuildSystem.BuildConfigs; }
		}

		public static int CurrentConfigIndex
		{
			get { return project.BuildSystem.CurrentConfigIndex; }
			set { project.BuildSystem.CurrentConfigIndex = value; }
		}

		public static BuildConfig CurrentBuildConfig
		{
			get { return project.BuildSystem.CurrentConfig; }
		}
	}
}
