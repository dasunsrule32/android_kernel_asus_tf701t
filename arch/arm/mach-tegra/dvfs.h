/*
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * Author:
 *	Colin Cross <ccross@google.com>
 *
 * Copyright (C) 2010-2012 NVIDIA CORPORATION. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _TEGRA_DVFS_H_
#define _TEGRA_DVFS_H_

#include <linux/of.h>

#define MAX_DVFS_FREQS	40
#define MAX_DVFS_TABLES	80
#define DVFS_RAIL_STATS_TOP_BIN	42

struct clk;
struct dvfs_rail;

/*
 * dvfs_relationship between to rails, "from" and "to"
 * when the rail changes, it will call dvfs_rail_update on the rails
 * in the relationship_to list.
 * when determining the voltage to set a rail to, it will consider each
 * rail in the relationship_from list.
 */
struct dvfs_relationship {
	struct dvfs_rail *to;
	struct dvfs_rail *from;
	int (*solve)(struct dvfs_rail *, struct dvfs_rail *);

	struct list_head to_node; /* node in relationship_to list */
	struct list_head from_node; /* node in relationship_from list */
	bool solved_at_nominal;
};

struct rail_stats {
	ktime_t time_at_mv[DVFS_RAIL_STATS_TOP_BIN + 1];
	ktime_t last_update;
	int last_index;
	bool off;
};

struct dvfs_rail {
	const char *reg_id;
	int min_millivolts;
	int max_millivolts;
	int nominal_millivolts;
	int step;
	bool jmp_to_zero;
	bool disabled;
	bool updating;
	bool resolving_to;

	struct list_head node;  /* node in dvfs_rail_list */
	struct list_head dvfs;  /* list head of attached dvfs clocks */
	struct list_head relationships_to;
	struct list_head relationships_from;
	struct regulator *reg;
	int millivolts;
	int new_millivolts;
	int offs_millivolts;
	bool suspended;
	bool dfll_mode;
	struct rail_stats stats;
};

struct dvfs_dfll_data {
	u32		tune0;
	u32		tune1;
	unsigned long	droop_rate_min;
	unsigned long	out_rate_min;
	unsigned long	max_rate_boost;
};

struct dvfs {
	/* Used only by tegra2_clock.c */
	const char *clk_name;
	int speedo_id;
	int process_id;

	/* Must be initialized before tegra_dvfs_init */
	int freqs_mult;
	unsigned long freqs[MAX_DVFS_FREQS];
	unsigned long *alt_freqs;
	const int *millivolts;
	const int *dfll_millivolts;
	struct dvfs_rail *dvfs_rail;
	bool auto_dvfs;

	/* Filled in by tegra_dvfs_init */
	int max_millivolts;
	int num_freqs;
	int min_millivolts;
	struct dvfs_dfll_data dfll_data;

	int cur_millivolts;
	unsigned long cur_rate;
	struct list_head node;
	struct list_head debug_node;
	struct list_head reg_node;
};

struct cpu_cvb_dvfs_parameters {
	unsigned long freq;
	int	c0;
	int	c1;
	int	c2;
};

struct cpu_cvb_dvfs {
	int speedo_id;
	int max_mv;
	int min_mv;
	int margin;
	int freqs_mult;
	int speedo_scale;
	int voltage_scale;
	struct cpu_cvb_dvfs_parameters cvb_table[MAX_DVFS_FREQS];
};

extern struct dvfs_rail *tegra_cpu_rail;
extern struct dvfs_rail *tegra_core_rail;

struct dvfs_data {
	struct dvfs_rail *rail;
	struct dvfs *tables;
	int *millivolts;
	unsigned int num_tables;
	unsigned int num_voltages;
};

#ifdef CONFIG_OF
typedef int (*of_tegra_dvfs_init_cb_t)(struct device_node *);
int of_tegra_dvfs_init(const struct of_device_id *matches);
#else
static inline int of_tegra_dvfs_init(const struct of_device_id *matches)
{ return -ENODATA; }
#endif

void tegra2_init_dvfs(void);
void tegra3_init_dvfs(void);
void tegra11x_init_dvfs(void);
int tegra_enable_dvfs_on_clk(struct clk *c, struct dvfs *d);
int dvfs_debugfs_init(struct dentry *clk_debugfs_root);
int tegra_dvfs_late_init(void);
int tegra_dvfs_init_rails(struct dvfs_rail *dvfs_rails[], int n);
void tegra_dvfs_add_relationships(struct dvfs_relationship *rels, int n);
void tegra_dvfs_rail_enable(struct dvfs_rail *rail);
void tegra_dvfs_rail_disable(struct dvfs_rail *rail);
bool tegra_dvfs_rail_updating(struct clk *clk);
void tegra_dvfs_rail_off(struct dvfs_rail *rail, ktime_t now);
void tegra_dvfs_rail_on(struct dvfs_rail *rail, ktime_t now);
void tegra_dvfs_rail_pause(struct dvfs_rail *rail, ktime_t delta, bool on);
struct dvfs_rail *tegra_dvfs_get_rail_by_name(const char *reg_id);
int tegra_dvfs_predict_millivolts(struct clk *c, unsigned long rate);
void tegra_dvfs_core_cap_enable(bool enable);
void tegra_dvfs_core_cap_level_set(int level);
int tegra_dvfs_alt_freqs_set(struct dvfs *d, unsigned long *alt_freqs);
int tegra_cpu_dvfs_alter(int edp_thermal_index, const cpumask_t *cpus,
			 bool before_clk_update, int cpu_event);

#ifndef CONFIG_ARCH_TEGRA_2x_SOC
int tegra_dvfs_rail_disable_prepare(struct dvfs_rail *rail);
int tegra_dvfs_rail_post_enable(struct dvfs_rail *rail);
#else
static inline int tegra_dvfs_rail_disable_prepare(struct dvfs_rail *rail)
{ return 0; }
static inline int tegra_dvfs_rail_post_enable(struct dvfs_rail *rail)
{ return 0; }
#endif
#ifdef CONFIG_ARCH_TEGRA_3x_SOC
void tegra_dvfs_age_cpu(int cur_linear_age);
#else
static inline void tegra_dvfs_age_cpu(int cur_linear_age)
{ return; }
#endif



#endif