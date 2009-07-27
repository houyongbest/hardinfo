/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2009 Leandro A. F. Pereira <leandro@hardinfo.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

static gchar *smb_shares_list = NULL;

void scan_samba_from_string(gchar *str, gsize length);
void scan_samba_usershares(void);

void
scan_samba(void)
{
    gchar *str;
    gsize length;
    
    if (smb_shares_list) {
        g_free(smb_shares_list);
        smb_shares_list = g_strdup("");
    }

    if (g_file_get_contents("/etc/samba/smb.conf",
                            &str, &length, NULL)) {
        scan_samba_from_string(str, length);
        g_free(str);                        
    }

    scan_samba_usershares();
}

void
scan_samba_usershares(void)
{
    FILE *usershare_list;
    
    if ((usershare_list = popen("net usershare list", "r"))) {
        char buffer[512];
        
        while (fgets(buffer, 512, usershare_list)) {
            gchar *usershare, *cmdline;
            gsize length;
            
            cmdline = g_strdup_printf("net usershare info %s",
                                      strend(buffer, '\n'));
            if (g_spawn_command_line_sync(cmdline,
                                          &usershare, NULL,
                                          NULL, NULL)) {
                length = strlen(usershare);
                scan_samba_from_string(usershare, length);
                g_free(usershare);
            }
            
            g_free(cmdline);
        }
        
        pclose(usershare_list);
    }
}

void
scan_samba_from_string(gchar *str, gsize length)
{
    GKeyFile *keyfile;
    GError *error = NULL;
    gchar **groups;
    gint i = 0;
    
    keyfile = g_key_file_new();
    
    gchar *_smbconf = str;
    for (; *_smbconf; _smbconf++)
        if (*_smbconf == ';') *_smbconf = '\0';
    
    if (!g_key_file_load_from_data(keyfile, str, length, 0, &error)) {
        smb_shares_list = g_strdup("Cannot parse smb.conf=\n");
        if (error)
            g_error_free(error);
        goto cleanup;
    }

    groups = g_key_file_get_groups(keyfile, NULL);
    while (groups[i]) {
        if (g_key_file_has_key(keyfile, groups[i], "path", NULL)) {
            gchar *path = g_key_file_get_string(keyfile, groups[i], "path", NULL);
            smb_shares_list = h_strdup_cprintf("%s=%s\n",
                                               smb_shares_list,
                                               groups[i], path);
            g_free(path);
        }
        
        i++;
    }
    
    g_strfreev(groups);
  
  cleanup:
    g_key_file_free(keyfile);
}

