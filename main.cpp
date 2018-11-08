//replace this with the mozilla backup file, recovery.jsonlz4
//maybe switch to a grid?
//find a way to show urls
//update listbox everytime
//update file_infos everytime
//add button fuctionality (replace, save)
//add preferences dialog

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "lz4.h"
#include "json.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <memory>
#include <wx/artprov.h>
#include <wx/dir.h>
#include <boost/filesystem.hpp> //check if I really need this at end
#include <ctime>
#include <stdio.h>


#ifdef _WIN32
wxString slash = "\\";
#else
wxString slash = "/";
#endif
/*
6 immediate ones
1 hr, 2 hrs, 3 hrs, 4 hrs, 5 hrs, 6 hrs,
1 day
3 day
1 week
2 weeks
1 month
3 month
6 month
1 yr
3 yr
5 yr
longest
*/
/*
 5
 10
 15
 20
 25
 30-gets erased unless 1 day reaches 3 days
 
 1 day-gets deleted when it reaches 3 days and slot is not open
 3 days-gets deleted when it reaches 1 week and slot is not open
 */
#ifdef WIN32
#define stat _stat
#else
#include <unistd.h>
#endif

struct pert_file_info
{
    time_t last_write_time;
    bool auto_created;
    std::string file_name;
};

bool compare_file_infos(pert_file_info& i, pert_file_info& j)
{
    return i.last_write_time < j.last_write_time;
}

union chars_to_int
{
    int equiv_int;
    char four_chars[4];
};

struct tab_info
{
    std::string title;
    std::string url;
};

bool after_specified_time(int number, std::string unit, std::time_t now, std::time_t time_to_compare)
{
    tm now_nice = *std::localtime(&now);
    tm time_to_compare_nice = *std::localtime(&time_to_compare);
    int years_since = now_nice.tm_year - time_to_compare_nice.tm_year;
    int months_since = now_nice.tm_mon - time_to_compare_nice. tm_mon;
    int days_since = now_nice.tm_mday - time_to_compare_nice.tm_mday;
    int hours_since = now_nice.tm_hour - time_to_compare_nice.tm_hour;
    int mins_since = now_nice.tm_min - time_to_compare_nice.tm_min;
    if (unit == "years")
    {
        if (years_since > number)
        {
            return true;
        }
        else if (years_since == number && months_since>0)
        {
            return true;
        }
    }
    else if (unit == "months")
    {
        if (months_since > number)
        {
            return true;
        }
        else if (months_since == number && days_since>0)
        {
            return true;
        }
    }
    else if (unit == "days")
    {
        if (days_since > number)
        {
            return true;
        }
        else if (days_since == number && hours_since>0)
        {
            return true;
        }
    }
    else if (unit == "hrs")
    {
        if (hours_since > number)
        {
            return true;
        }
        else if (hours_since == number && mins_since>0)
        {
            return true;
        }
    }
    else if (unit == "mins")
    {
        if (mins_since >= number)
        {
            return true;
        }
    }
    return false;
}

class session_app: public wxApp
{
public:
    virtual bool OnInit();
};
class recovery_panel : public wxPanel
{
public:
    recovery_panel(wxPanel* parent) : wxPanel(parent, -1, wxPoint(-1, -1), wxSize(-1, -1), wxBORDER_SUNKEN)
    {
        panel_parent = parent;
        
    }
    wxPanel* panel_parent;
    std::vector<wxButton*> recovery_options;
};
class tab_panel : public wxPanel
{
public:
    tab_panel(wxPanel* parent) : wxPanel(parent, -1, wxPoint(-1, -1), wxSize(-1, -1), wxBORDER_SUNKEN)
    {
        panel_parent = parent;
    }
    wxPanel* panel_parent;
};

//this is for the backup directory
std::vector<pert_file_info> get_important_file_info (std::string path_to_jsonlz4s)
{
    wxString wx_path_to_jsonlz4s = wxString(path_to_jsonlz4s);
    wxDir jsonlz4_dir(wx_path_to_jsonlz4s);
    wxString filename;
    std::vector<pert_file_info> to_return;
    bool file_found = jsonlz4_dir.GetFirst(&filename, "*.jsonlz4");
    while (file_found)
    {
        pert_file_info temp;
        temp.file_name = filename;
        std::string equiv_string = filename.ToStdString();
        boost::filesystem::path path_to_file = path_to_jsonlz4s+slash.ToStdString()+equiv_string;
        temp.last_write_time = boost::filesystem::last_write_time(path_to_file);
        if (equiv_string.find("autosave") != std::string::npos)
        {
            temp.auto_created=true;
        }
        else
        {
            temp.auto_created=false;
        }
        to_return.push_back(temp);
        file_found = jsonlz4_dir.GetNext(&filename);
    }
    std::sort(to_return.begin(), to_return.end(), compare_file_infos);
    return to_return;
    //oldest is first
}

class application_window : public wxFrame
{
public:
    application_window(const wxString& title, const wxSize& size, nlohmann::json& in_prefs) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, size)
    {
        loaded_preferences = in_prefs;
        file_infos = get_important_file_info(loaded_preferences["backup_path"].get<std::string>());
        wxMenu* file_dropdown = new wxMenu;
        file_dropdown->Append(wxID_EXIT, "&Help");
        file_dropdown->Append(wxID_EXIT, "&Credits");
        file_dropdown->Append(wxID_EXIT, "&Preferences");
        file_dropdown->AppendSeparator();
        file_dropdown->Append(wxID_EXIT, "&Quit");
        wxMenuBar* menu_bar = new wxMenuBar;
        menu_bar->Append(file_dropdown, wxT("File"));
        SetMenuBar(menu_bar);
        CreateStatusBar();
        SetStatusText("Man I wish Firefox still had a status bar");
        panel_parent = new wxPanel(this, wxID_ANY);
        
        wxImage::AddHandler(new wxPNGHandler);
        wxBitmap preferences(wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE, wxART_TOOLBAR));
        wxBitmap help(wxArtProvider::GetBitmap(wxART_HELP, wxART_TOOLBAR));
        wxBitmap credits(wxArtProvider::GetBitmap(wxART_HELP_PAGE, wxART_TOOLBAR));
        wxBitmap open(wxArtProvider::GetBitmap(wxART_FOLDER_OPEN, wxART_BUTTON));

        wxBoxSizer* vert_box_sizer = new wxBoxSizer(wxVERTICAL);
        
        wxStaticText* title_text = new wxStaticText(panel_parent, wxID_ANY, wxT("Session JSON Saver"));
        vert_box_sizer->Add(title_text, 0, wxALIGN_CENTER | wxALL, 5);
        
        wxFlexGridSizer* folders_and_paths = new wxFlexGridSizer(2, 3, 5, 5);
        wxStaticText* mozilla_path = new wxStaticText(panel_parent, wxID_ANY, wxT("Mozilla Session Folder: "));
        mozilla_path_text = new wxTextCtrl(panel_parent, wxID_ANY,
                                           wxString(loaded_preferences["mozilla_path"].get<std::string>()),
                                           wxPoint(-1, -1), wxSize(-1, -1), wxTE_READONLY);
        wxStaticText* backup_path = new wxStaticText(panel_parent, wxID_ANY, wxT("Backups Folder: "));
        backup_path_text = new wxTextCtrl(panel_parent, wxID_ANY,
                                          wxString(loaded_preferences["backup_path"].get<std::string>()),
                                          wxPoint(-1, -1), wxSize(-1, -1), wxTE_READONLY);
        mozilla_file_dialog_button = new wxButton(panel_parent, ID_mozilla_button, wxT(""), wxPoint(-1, -1), wxSize(32, 28));
        backup_file_dialog_button =  new wxButton(panel_parent, ID_backup_button, wxT(""), wxPoint(-1, -1), wxSize(32, 28));
        Bind(wxEVT_COMMAND_BUTTON_CLICKED, &application_window::open_mozilla_file_dialog, this, ID_mozilla_button);
        Bind(wxEVT_COMMAND_BUTTON_CLICKED, &application_window::open_backup_file_dialog, this, ID_backup_button);
        mozilla_file_dialog_button->SetBitmap(open);
        backup_file_dialog_button->SetBitmap(open);
        folders_and_paths->Add(mozilla_path);
        folders_and_paths->Add(mozilla_path_text, 1, wxEXPAND);
        folders_and_paths->Add(mozilla_file_dialog_button, 0);
        folders_and_paths->Add(backup_path);
        folders_and_paths->Add(backup_path_text, 1, wxEXPAND);
        folders_and_paths->Add(backup_file_dialog_button, 0);
        folders_and_paths->AddGrowableCol(1,1);
        vert_box_sizer->Add(folders_and_paths, 0, wxEXPAND| wxALL, 5);
        
        wxBoxSizer* jsons_and_tabs_box_sizer = new wxBoxSizer(wxHORIZONTAL);
        wxBoxSizer* jsons_box_sizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* tabs_box_sizer = new wxBoxSizer(wxVERTICAL);
        wxStaticText* jsons_title = new wxStaticText(panel_parent, wxID_ANY, wxT("Session JSONs"));
        wxStaticText* tabs_title = new wxStaticText(panel_parent, wxID_ANY, wxT("Windows and Tabs in Session"));
        json_listbox = new wxListBox(panel_parent, ID_json_listbox, wxPoint(-1, -1), wxSize(300, -1));
        for (pert_file_info& file : file_infos)
        {
            std::tm* mod_time = std::localtime(&file.last_write_time);
            //maybe have an option for date formats
            int two_digit_year;
            if (mod_time->tm_year>=100)
            {
                two_digit_year=mod_time->tm_year-100;
            }
            else
            {
                two_digit_year=mod_time->tm_year;
            }
            wxString hour_minute = wxString::Format(wxT("%i:%i"), mod_time->tm_hour, mod_time->tm_min);
            wxString month_year_date = wxString::Format(wxT("%i/%i/%i"), mod_time->tm_mon+1, mod_time->tm_mday, two_digit_year);
            json_listbox->Append(file.file_name + " saved at " + hour_minute + " on " + month_year_date);
        }
        
        Bind(wxEVT_COMMAND_LISTBOX_SELECTED, &application_window::json_listbox_click, this, ID_json_listbox);
        tab_listbox = new wxListBox(panel_parent, ID_tab_listbox, wxPoint(-1, -1), wxSize(-1, -1));
        
        jsons_box_sizer->Add(jsons_title, 0, wxBOTTOM, 5);
        jsons_box_sizer->Add(json_listbox, 1, wxEXPAND);
        tabs_box_sizer->Add(tabs_title, 0, wxBOTTOM, 5);
        tabs_box_sizer->Add(tab_listbox, 1, wxEXPAND);
        jsons_and_tabs_box_sizer->Add(jsons_box_sizer, 1, wxEXPAND | wxALL, 5);
        jsons_and_tabs_box_sizer->Add(tabs_box_sizer, 2, wxEXPAND | wxALL, 5);
        vert_box_sizer->Add(jsons_and_tabs_box_sizer, 1, wxEXPAND | wxALL, 10);
        
        
        wxButton* replace_button = new wxButton(panel_parent, wxID_ANY, wxT("Replace!"));
        vert_box_sizer->Add(replace_button, 0, wxALIGN_RIGHT | wxALL, 5);
        
        panel_parent->SetSizer(vert_box_sizer);
        
        autosave_timer = new wxTimer(this, ID_timer);
        int milliseconds_between = loaded_preferences["autosave freq (minutes)"].get<int>() * 60 * 1000;
        autosave_timer->Start(milliseconds_between);
        Bind(wxEVT_TIMER, &application_window::autosave, this, ID_timer);
        this->Center();
        wxToolBar* toolbar = CreateToolBar();
        toolbar->AddStretchableSpace();
        toolbar->AddTool(wxID_EXIT, wxT("Help"), help);
        toolbar->AddTool(wxID_EXIT, wxT("Credits"), credits);
        toolbar->AddTool(wxID_EXIT, wxT("Preferences"), preferences);
        toolbar->Realize();
        check_for_deletions();
    }
    /*
     if (sel== 0)
     {
     std::cout<<"real";
     }
     std::ifstream input(loaded_preferences["mozilla_path"].get<std::string>()+slash.ToStdString()+file_infos[sel].file_name);
     std::string line;
     std::string compressed_session_info;
     input>>compressed_session_info;
     //8 chars of "mozlz40 ", 4 chars worth of decompressed size
     int magic_numbers=8;
     int decompressed_size_numbers=4;
     std::string decomp_size_string = compressed_session_info.substr(magic_numbers, decompressed_size_numbers);
     compressed_session_info = compressed_session_info.substr((magic_numbers+decompressed_size_numbers));
     chars_to_int get_decomp_size;
     for (int i=0; i<decompressed_size_numbers; i++)
     {
     get_decomp_size.four_chars[i] = decomp_size_string[i];
     }
     input.close();
     int destination_capacity = get_decomp_size.equiv_int;
     std::cout << "\n" + std::to_string(compressed_session_info.length()) + "\n";
     char* destination = new char[destination_capacity];
     int returned_num = LZ4_decompress_safe(compressed_session_info.data(), destination, compressed_session_info.size(), destination_capacity);
     */

private:
    wxPanel* panel_parent;
    wxListBox* json_listbox;
    wxListBox* tab_listbox;
    recovery_panel* file_panel;
    tab_panel* tab_info_panel;
    wxButton* mozilla_file_dialog_button;
    wxButton* backup_file_dialog_button;
    wxTextCtrl* mozilla_path_text;
    wxTextCtrl* backup_path_text;
    nlohmann::json loaded_preferences;
    std::vector<pert_file_info> file_infos;
    wxTimer* autosave_timer;
    const int ID_mozilla_button = 101;
    const int ID_backup_button = 102;
    const int ID_json_listbox = 103;
    const int ID_tab_listbox = 104;
    const int ID_timer = 105;
    void on_exit(wxCommandEvent& event)
    {
        Close(true);
    }
    void json_listbox_click(wxCommandEvent& event)
    {
        /*
         if (sel != -1)
         {
         
         }
         */
        tab_listbox->Clear();
        int sel = json_listbox->GetSelection();
        if (sel== 0)
        {
            std::cout<<"real";
        }
        std::ifstream input(loaded_preferences["backup_path"].get<std::string>()+slash.ToStdString()+file_infos[sel].file_name);
        std::string compressed_session_info;
        char c = input.get();
        //8 chars of "mozlz40 ", 4 chars worth of decompressed size
        int magic_numbers=8;
        int decompressed_size=4;
        int counter=0;
        chars_to_int get_decomp_size;
        while (input.good()) {
            if (counter<(magic_numbers+decompressed_size))
            {
                if(counter>=magic_numbers)
                {
                    get_decomp_size.four_chars[counter-magic_numbers]=c;
                }
                c = input.get();
                counter+=1;
            }
            else
            {
                compressed_session_info+=c;
                c = input.get();
            }
        }
        input.close();
        int destination_capacity = get_decomp_size.equiv_int;
        char* destination = new char[destination_capacity];
        int returned_num = LZ4_decompress_safe(compressed_session_info.data(), destination, compressed_session_info.size(), destination_capacity);
        std::vector<char> destination_holder(destination, destination+destination_capacity);
        delete[] destination;
        
        std::string uncompressed_json;
        for (char & letter : destination_holder)
        {
            uncompressed_json+=letter;
        }
        auto session_info = nlohmann::json::parse(uncompressed_json);
        int window_num=1;
        for (auto & window : session_info["windows"])
        {
            tab_listbox->Append("Window " + std::to_string(window_num));
            for (auto & tab : window["tabs"])
            {
                int active_entry = tab["index"];
                auto & current_entry = tab["entries"][(active_entry-1)];
                if (current_entry.find("title") != current_entry.end())
                {
                    std::string current_tab_name = current_entry["title"];
                    tab_listbox->Append(current_tab_name);
                    std::cout<<current_tab_name<<"\n";
                }
                else if (current_entry.find("url") != current_entry.end())
                {
                    std::string current_tab_name = current_entry["url"];
                    tab_listbox->Append(current_tab_name);
                    std::cout<<current_tab_name<<"\n";
                }
                else
                {
                    std::string current_tab_name = "No Title or Url found";
                    tab_listbox->Append(current_tab_name);
                    std::cout<<current_tab_name<<"\n";
                }
            }
            window_num+=1;
        }
        /*
        struct stat result;
         std::ifstream  src("from.ogv", std::ios::binary);
         std::ofstream  dst("to.ogv",   std::ios::binary);
         
         dst << src.rdbuf();
         */
        
        std::cout<<"cool";
    }
    void open_backup_file_dialog(wxCommandEvent& event)
    {
        wxDirDialog * open_file_dialog = new wxDirDialog(this);
        if (open_file_dialog->ShowModal() == wxID_OK){
            wxString file_name = open_file_dialog->GetPath();
            backup_path_text->SetValue(file_name);
        }
        std::cout<<"ayo";
    }
    void open_mozilla_file_dialog(wxCommandEvent& event)
    {
        wxDirDialog * open_file_dialog = new wxDirDialog(this);
        if (open_file_dialog->ShowModal() == wxID_OK){
            wxString file_name = open_file_dialog->GetPath();
            mozilla_path_text->SetValue(file_name);
        }
        std::cout<<"watup";
    }
    //split this to autosave and check for deletions
    void check_for_deletions()
    {
        file_infos = get_important_file_info(loaded_preferences["backup_path"].get<std::string>());
        std::time_t current_time = std::time(0);
        std::vector<pert_file_info*> auto_created_file_infos; //try to assign one of each to preference?
        int preference_slot_filled = 0;
        std::vector<std::string> marked_for_deletion;
        int auto_save_slots = loaded_preferences["save times"].size();
        std::cout<<auto_save_slots;
        for (pert_file_info& file : file_infos)
        {
            if (file.auto_created)
            {
                auto_created_file_infos.push_back(&file);
            }
        }
        int previous_filled = 0;
        bool has_deleted = false;
        while (preference_slot_filled < auto_save_slots && preference_slot_filled < auto_created_file_infos.size()) //goes upwards
        {
            has_deleted=false;
            int number_of_units = loaded_preferences["save times"][auto_save_slots-preference_slot_filled-1].get<int>();
            std::string time_unit = loaded_preferences["save units"][auto_save_slots-preference_slot_filled-1].get<std::string>();
            if (time_unit != "indef") //automatically takes it if it's indef
            {
                for (int i=preference_slot_filled; i<auto_created_file_infos.size(); ++i)
                {
                    pert_file_info file = *auto_created_file_infos[i];
                    if (after_specified_time(number_of_units, time_unit, current_time, file.last_write_time))
                    {
                        marked_for_deletion.push_back(file.file_name);
                        has_deleted=true;
                    }
                    else
                    {
                        if (marked_for_deletion.size()>0 && has_deleted)
                        {
                            previous_filled=i-1;
                            marked_for_deletion.pop_back();    //once it reaches a file age younger, it chooses the one right before to keep
                        }
                        else
                        {
                            previous_filled=i;
                        }
                        break;
                    }
                }
            }
            preference_slot_filled+=1;
        }
        std::cout<< "\n" << marked_for_deletion.size() << " : ";
        std::cout<<file_infos.size() <<" : ";
        for (std::string file_name : marked_for_deletion)
        {
            std::string path_to_file = loaded_preferences["backup_path"].get<std::string>() + slash.ToStdString() + file_name;
            remove(path_to_file.c_str());
        }
        if (previous_filled<(auto_created_file_infos.size()-1)) //delete extras at the end
        {
            for (int i=previous_filled; i<auto_created_file_infos.size()-1; i++)
            {
                std::string to_delete_name = auto_created_file_infos[i]->file_name;
                std::string path_to_file = loaded_preferences["backup_path"].get<std::string>() + slash.ToStdString() + to_delete_name;
                remove(path_to_file.c_str());
            }
        }
        file_infos = get_important_file_info(loaded_preferences["backup_path"].get<std::string>());
        std::cout<<file_infos.size() <<"\n";
        json_listbox->Clear();
        for (pert_file_info& file : file_infos)
        {
            std::tm* mod_time = std::localtime(&file.last_write_time);
            //maybe have an option for date formats
            int two_digit_year;
            if (mod_time->tm_year>=100)
            {
                two_digit_year=mod_time->tm_year-100;
            }
            else
            {
                two_digit_year=mod_time->tm_year;
            }
            wxString hour_minute = wxString::Format(wxT("%i:%i"), mod_time->tm_hour, mod_time->tm_min);
            wxString month_year_date = wxString::Format(wxT("%i/%i/%i"), mod_time->tm_mon+1, mod_time->tm_mday, two_digit_year);
            json_listbox->Append(file.file_name + " saved at " + hour_minute + " on " + month_year_date);
        }
    }
    void autosave(wxTimerEvent& event)
    {
        std::time_t current_time = std::time(0);
        tm* current_time_nice = std::localtime(&current_time);
        std::string to_copy_name = loaded_preferences["mozilla_path"].get<std::string>() + slash.ToStdString() + "recovery.jsonlz4";
        std::string destination_name = loaded_preferences["backup_path"].get<std::string>() + slash.ToStdString() + "autosave " + asctime(current_time_nice)+".jsonlz4";
        std::ifstream src(to_copy_name, std::ios::binary);
        std::ofstream dst(destination_name, std::ios::binary);
        dst << src.rdbuf();
        check_for_deletions();
    }
    wxDECLARE_EVENT_TABLE();
};

nlohmann::json read_preferences()
{
    if (wxFileExists("preferences.json"))
    {
        std::ifstream existing_preferences ("preferences.json");
        nlohmann::json to_return;
        existing_preferences>>to_return;
        return to_return;
    }
    else
    {
        //defaults in case this fails
        wxString mozilla_path = wxGetCwd();
        boost::filesystem::create_directory("jsonlz4-backups");
        wxString backup_path = wxGetCwd()+slash+wxString("jsonlz4-backups");
        wxString user_data_dir = wxPathOnly(wxStandardPaths::Get().GetUserDataDir());
        wxString mozilla_profile;
        bool found_profile_folder=false;
        if (wxDirExists(user_data_dir+slash+wxString("Firefox")+slash+wxString("Profiles")))
        {
            mozilla_profile = user_data_dir+slash+wxString("Firefox")+slash+wxString("Profiles");
            found_profile_folder=true;
        }
        else if (wxDirExists(user_data_dir+slash+wxString("Mozilla")+slash+wxString("Firefox")+slash+wxString("Profiles")))
        {
            mozilla_profile =user_data_dir+slash+wxString("Mozilla")+slash+wxString("Firefox")+slash+wxString("Profiles");
            found_profile_folder=true;
        }
        std::cout<<mozilla_profile.ToStdString();
        if (found_profile_folder)
        {
            bool file_find_success;
            wxString first_profile_name;
            wxDir mozilla_profile_directory(mozilla_profile);
            file_find_success= mozilla_profile_directory.GetFirst(&first_profile_name, wxEmptyString, wxDIR_DIRS);
            if (file_find_success)
            {
                if (wxDirExists(mozilla_profile+slash+first_profile_name+slash+wxString("sessionstore-backups")))
                {
                    mozilla_path=mozilla_profile+slash+first_profile_name+slash+wxString("sessionstore-backups");
                }
            }
        }
        nlohmann::json init_prefs;
        init_prefs["mozilla_path"] = mozilla_path.ToStdString();
        init_prefs["backup_path"] = backup_path.ToStdString();
        //minutes, days, months, years, forever
        //one forced autosave
        init_prefs["autosave freq (minutes)"]=5; //if this is less than first save time, save one
        init_prefs["save times"] =
        {
            5, 10, 15, 20, 25, 30,
            1, 2, 3, 4, 5, 6,
            1, 3, 7, 14,
            1, 3, 6,
            1, 3, 5,
            0
        }; //dialog makes sure this makes sense, makes sure it's in right order
        //automanaged ones and saved ones
        init_prefs["save units"] =
        {
            "mins", "mins", "mins", "mins", "mins", "mins",
            "hrs", "hrs", "hrs", "hrs", "hrs", "hrs",
            "days", "days", "days", "days",
            "months", "months", "months",
            "years", "years", "years",
            "indef"
        };
        std::ofstream writing_preferences("preferences.json");
        writing_preferences<< init_prefs.dump(4);
        return init_prefs;
    }
}
bool session_app::OnInit()
{
    nlohmann::json preferences = read_preferences();
    application_window *window = new application_window("Session JSON saver", wxSize(800, 500), preferences );
    window->Show(true);
    return true;
}


wxBEGIN_EVENT_TABLE(application_window, wxFrame)
EVT_MENU(wxID_EXIT, application_window::on_exit)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(session_app);


/*
int main(int argc, const char * argv[]) {
    std::string pathname_to_recovery_file = get_pathname()+"/recovery.jsonlz4";
    std::ifstream input(pathname_to_recovery_file);
    std::string line;
    std::string compressed_session_info;
    char c = input.get();
    //8 chars of "mozlz40 ", 4 chars worth of decompressed size
    int magic_numbers=8;
    int decompressed_size=4;
    int counter=0;
    chars_to_int get_decomp_size;
    while (input.good()) {
        if (counter<(magic_numbers+decompressed_size))
        {
            if(counter>=magic_numbers)
            {
                get_decomp_size.four_chars[counter-magic_numbers]=c;
            }
            c = input.get();
            counter+=1;
        }
        else
        {
            compressed_session_info+=c;
            c = input.get();
        }
    }
    input.close();
    int destination_capacity = get_decomp_size.equiv_int;
    char* destination = new char[destination_capacity];
    int returned_num = LZ4_decompress_safe(compressed_session_info.data(), destination, compressed_session_info.size(), destination_capacity);
    std::ofstream output("actual_text.json");
    std::vector<char> destination_holder(destination, destination+destination_capacity);
    delete[] destination;
    
    std::string uncompressed_json;
    for (char & letter : destination_holder)
    {
        uncompressed_json+=letter;
    }
    auto session_info = nlohmann::json::parse(uncompressed_json);
    for (auto & window : session_info["windows"])
    {
        for (auto & tab : window["tabs"])
        {
            int active_entry = tab["index"];
            auto & current_entry = tab["entries"][(active_entry-1)];
            if (current_entry.find("title") != current_entry.end())
            {
                std::string current_tab_name = current_entry["title"];
                std::cout<<current_tab_name<<"\n";
            }
            else if (current_entry.find("url") != current_entry.end())
            {
                std::string current_tab_name = current_entry["url"];
                std::cout<<current_tab_name<<"\n";
            }
            else
            {
                std::string current_tab_name = "No Title or Url found";
                std::cout<<current_tab_name<<"\n";
            }
        }
    }
    output << session_info.dump(4);
    struct stat result;
    /*
     std::ifstream  src("from.ogv", std::ios::binary);
     std::ofstream  dst("to.ogv",   std::ios::binary);
     
     dst << src.rdbuf();
     */
//    return 0;
//}
