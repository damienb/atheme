/*
 * Copyright (c) 2010 William Pitcock <nenolod@atheme.org>.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * ACCESS command implementation for ChanServ.
 */

#include "atheme.h"
#include "template.h"

DECLARE_MODULE_V1
(
	"chanserv/access", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[]);
static void cs_help_access(sourceinfo_t *si, char *subcmd);

command_t cs_access = { "ACCESS", N_("Manage channel access."),
                        AC_NONE, 3, cs_cmd_access, { .func = cs_help_access } };

static void cs_cmd_access_list(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_list = { "LIST", N_("List channel access entries."),
                             AC_NONE, 1, cs_cmd_access_list, { .path = "cservice/access_list" } };

static void cs_cmd_access_info(sourceinfo_t *si, int parc, char *parv[]);

command_t cs_access_info = { "INFO", N_("Display information on an access list entry."),
                             AC_NONE, 2, cs_cmd_access_info, { .path = "cservice/access_info" } };

mowgli_patricia_t *cs_access_cmds;

void _modinit(module_t *m)
{
	service_named_bind_command("chanserv", &cs_access);

	cs_access_cmds = mowgli_patricia_create(strcasecanon);

	command_add(&cs_access_list, cs_access_cmds);
	command_add(&cs_access_info, cs_access_cmds);
}

void _moddeinit()
{
	service_named_unbind_command("chanserv", &cs_access);

	mowgli_patricia_destroy(cs_access_cmds, NULL, NULL);
}

static void cs_help_access(sourceinfo_t *si, char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), chansvs.me->disp);
		command_success_nodata(si, _("Help for \2ACCESS\2:"));
		command_success_nodata(si, " ");
		command_help(si, cs_access_cmds);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more information, use \2/msg %s HELP ACCESS \37command\37\2."), chansvs.me->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display(si, si->service, subcmd, cs_access_cmds);
}

static void cs_cmd_access(sourceinfo_t *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	command_t *c;
	char buf[BUFSIZE];

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> <command> [parameters]"));
		return;
	}
	
	if (parv[0][0] == '#')
		chan = parv[0], cmd = parv[1];
	else if (parv[1][0] == '#')
		cmd = parv[0], chan = parv[1];
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "ACCESS");
		command_fail(si, fault_badparams, _("Syntax: ACCESS <#channel> <command> [parameters]"));
		return;
	}

	c = command_find(cs_access_cmds, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
		return;
	}

	if (parc > 2)
		snprintf(buf, BUFSIZE, "%s %s", chan, parv[2]);
	else
		strlcpy(buf, chan, BUFSIZE);

	command_exec_split(si->service, si, c->name, buf, cs_access_cmds);
}

/***********************************************************************************************/

static const char *get_template_name_fuzzy(mychan_t *mc, unsigned int level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if ((level & flags_to_bitmask(ss, 0)) == flags_to_bitmask(ss, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}

	if ((level & chansvs.ca_sop) == chansvs.ca_sop || (level & get_template_flags(mc, "SOP")) == get_template_flags(mc, "SOP"))
		return "SOP";
	if ((level & chansvs.ca_aop) == chansvs.ca_aop || (level & get_template_flags(mc, "AOP")) == get_template_flags(mc, "AOP"))
		return "AOP";
	/* if vop==hop, prefer vop */
	if ((level & chansvs.ca_vop) == chansvs.ca_vop || (level & get_template_flags(mc, "VOP")) == get_template_flags(mc, "VOP"))
		return "VOP";
	if (chansvs.ca_hop != chansvs.ca_vop && ((level & chansvs.ca_hop) == chansvs.ca_hop ||
			(level & get_template_flags(mc, "HOP")) == get_template_flags(mc, "HOP")))
		return "HOP";
	return NULL;
}

static const char *get_template_name(mychan_t *mc, unsigned int level)
{
	metadata_t *md;
	const char *p, *q, *r;
	char *s;
	char ss[40];
	static char flagname[400];

	md = metadata_find(mc, "private:templates");
	if (md != NULL)
	{
		p = md->value;
		while (p != NULL)
		{
			while (*p == ' ')
				p++;
			q = strchr(p, '=');
			if (q == NULL)
				break;
			r = strchr(q, ' ');
			if (r != NULL && r < q)
				break;
			strlcpy(ss, q, sizeof ss);
			if (r != NULL && r - q < (int)(sizeof ss - 1))
			{
				ss[r - q] = '\0';
			}
			if (level == flags_to_bitmask(ss, 0))
			{
				strlcpy(flagname, p, sizeof flagname);
				s = strchr(flagname, '=');
				if (s != NULL)
					*s = '\0';
				return flagname;
			}
			p = r;
		}
	}
	if (level == chansvs.ca_sop && level == get_template_flags(mc, "SOP"))
		return "SOP";
	if (level == chansvs.ca_aop && level == get_template_flags(mc, "AOP"))
		return "AOP";
	/* if vop==hop, prefer vop */
	if (level == chansvs.ca_vop && level == get_template_flags(mc, "VOP"))
		return "VOP";
	if (chansvs.ca_hop != chansvs.ca_vop && level == chansvs.ca_hop &&
			level == get_template_flags(mc, "HOP"))
		return "HOP";
	return NULL;
}

static const char *get_role_name(mychan_t *mc, unsigned int level)
{
	const char *role;

	if ((role = get_template_name(mc, level)) != NULL)
		return role;

	return get_template_name_fuzzy(mc, level);
}

/***********************************************************************************************/

/*
 * Syntax: ACCESS #channel LIST
 *
 * Output:
 *
 * Entry Nickname/Host          Role
 * ----- ---------------------- ----
 * 1     nenolod                founder
 * 2     jdhore                 channel-staffer
 */
static void cs_cmd_access_list(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	node_t *n;
	mychan_t *mc;
	const char *channel = parv[0];
	int operoverride = 0;
	unsigned int i = 1;

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = 1;
		else
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}

	command_success_nodata(si, _("Entry Nickname/Host          Role"));
	command_success_nodata(si, "----- ---------------------- ----");

	LIST_FOREACH(n, mc->chanacs.head)
	{
		const char *role;

		ca = n->data;
		role = get_role_name(mc, ca->level);

		command_success_nodata(si, _("%-5d %-22s %s"), i, ca->entity ? ca->entity->name : ca->host, role);

		i++;
	}

	command_success_nodata(si, "----- ---------------------- ----");
	command_success_nodata(si, _("End of \2%s\2 ACCESS listing."), channel);

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:LIST: \2%s\2 (oper override)", mc->name);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:LIST: \2%s\2", mc->name);
}

/*
 * Syntax: ACCESS #channel INFO user
 *
 * Output:
 *
 * Access for nenolod in #atheme:
 * Role       : Founder
 * Flags      : aclview, ...
 * Modified   : Aug 18 21:41:31 2005 (5 years, 6 weeks, 0 days, 05:57:21 ago)
 * *** End of Info ***
 */
static void cs_cmd_access_info(sourceinfo_t *si, int parc, char *parv[])
{
	chanacs_t *ca;
	myentity_t *mt;
	mychan_t *mc;
	const char *channel = parv[0];
	const char *target = parv[1];
	int operoverride = 0;
	unsigned int i = 1;
	const char *role;
	struct tm tm;
	char strfbuf[BUFSIZE];

	mc = mychan_find(channel);
	if (!mc)
	{
		command_fail(si, fault_nosuch_target, _("Channel \2%s\2 is not registered."), channel);
		return;
	}

	if (!target)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "ACCESS INFO");
		command_fail(si, fault_needmoreparams, _("Syntax: ACCESS <#channel> INFO <account|hostmask>"));
		return;
	}

	if (!chanacs_source_has_flag(mc, si, CA_ACLVIEW))
	{
		if (has_priv(si, PRIV_CHAN_AUSPEX))
			operoverride = 1;
		else
		{
			command_fail(si, fault_noprivs, _("You are not authorized to perform this operation."));
			return;
		}
	}

	if (validhostmask(target))
		ca = chanacs_find_host_literal(mc, target, 0);
	else
	{
		if (!(mt = myentity_find(target)))
		{
			command_fail(si, fault_nosuch_target, _("\2%s\2 is not registered."), target);
			return;
		}
		target = mt->name;
		ca = chanacs_find(mc, mt, 0);
	}

	if (ca == NULL)
	{
		command_success_nodata(si, _("No ACL entry for \2%s\2 in \2%s\2 was found."), target, channel);

		if (operoverride)
			logcommand(si, CMDLOG_ADMIN, "ACCESS:INFO: \2%s\2 on \2%s\2 (oper override)", mc->name, target);
		else
			logcommand(si, CMDLOG_GET, "ACCESS:INFO: \2%s\2 on \2%s\2", mc->name, target);

		return;
	}

	role = get_role_name(mc, ca->level);

	tm = *localtime(&ca->tmodified);
	strftime(strfbuf, sizeof(strfbuf) - 1, "%b %d %H:%M:%S %Y", &tm);

	command_success_nodata(si, _("Access for \2%s\2 in \2%s\2:"), target, channel);
	command_success_nodata(si, _("Role       : %s"), role);
	command_success_nodata(si, _("Privileges : %s"), xflag_tostr(ca->level));
	command_success_nodata(si, _("Flags      : %s"), bitmask_to_flags2(ca->level, 0));
	command_success_nodata(si, _("Modified   : %s (%s ago)"), strfbuf, time_ago(ca->tmodified));
	command_success_nodata(si, _("*** \2End of Info\2 ***"));

	if (operoverride)
		logcommand(si, CMDLOG_ADMIN, "ACCESS:INFO: \2%s\2 on \2%s\2 (oper override)", mc->name, target);
	else
		logcommand(si, CMDLOG_GET, "ACCESS:INFO: \2%s\2 on \2%s\2", mc->name, target);
}

/* vim:cinoptions=>s,e0,n0,f0,{0,}0,^0,=s,ps,t0,c3,+s,(2s,us,)20,*30,gs,hs
 * vim:ts=8
 * vim:sw=8
 * vim:noexpandtab
 */