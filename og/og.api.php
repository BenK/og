<?php
// $Id$

/**
 * @file
 * Hooks provided by the Organic groups module.
 */

/**
 * @addtgrouproup hooks
 * @{
 */

/**
 * Add group permissions.
 */
function hook_og_permission() {
  return array(
    'subscribe' => array(
      'title' => t('Subscribe user to group'),
      'description' => t("Allow user to be a member of a group (approval required)."),
      'roles' => array(OG_ANONYMOUS_ROLE),
    ),
  );
}

/**
 * Alter the organic groups permissions.
 *
 * @param $perms
 *   The permissions passed by reference.
 */
function hook_og_permission_alter(&$perms) {

}

/**
 * Define group context handlers.
 *
 * @return
 *   Array keyed with the context handler name and an array of properties:
 *   - callback: The callback function that should return a an array of group
 *     IDs.
 *   - menu: TRUE indicates that the handler will try to find a context by the
 *     current menu item. Defaults to TRUE.
 *   - menu path: If "menu" property is TRUE, this property is required.
 *     An array of path the handler should be invoked. For example,
 *     if the user is viewing a node, the menu system is "node/%", and all
 *     group context handlers with this matching path, will be invoked.
 *   - priority: Optional; Indicate if the context result of this handler should
 *     be treated as a priority. A use case can be for example, the "session"
 *     context handler that returns the group context that is stored in the
 *     $_SESSION. By giving it a priority, we make sure that even if viewing
 *     different pages, the user will see the same group context.
 *     @see og_context_handler_session().
 */
function hook_og_context_handlers() {
  $items = array();

  $items['foo'] = array(
    'callback' => 'foo_context_handler_bar',
    'menu path' => array('foo/%', 'foo/%/bar'),
  );

  return $items;
}

/**
 * Alter the group context handlers.
 */
function hook_og_context_handlers_alter(&$items) {
  // Add another menu path that should invoke this handler.
  $items['foo']['menu path'][] = 'foo/%/baz';
}


/**
 * Set a default role that will be used as a global role.
 *
 * A global role, is a role that is assigned by default to all new groups.
 */
function hook_og_default_roles() {
  return array('super admin');
}

/**
 * Alter the default roles.
 *
 * The anonymous and authenticated member roles are not alterable.
 *
 * @param $roles
 *   Array with the default roles name.
 */
function hook_og_default_roles_alter(&$roles) {
  // Remove a default role.
  unset($roles['super admin']);
}

/**
 * Allow modules to act upon new group role.
 *
 * @param $role
 *   The group role object.
 */
function hook_og_user_role_insert($role) {
}

/**
 * Allow modules to act upon existing group role update.
 *
 * @param $role
 *   The group role object.
 */
function hook_og_user_role_update($role) {

}

/**
 * Allow modules to act upon existing group role deletion.
 *
 * @param $role
 *   The deleted group role object. The object is actually a dummy, as the data
 *   is already deleted from the database. However, we pass the object to allow
 *   implementing modules to properly identify the deleted role.
 */
function hook_og_user_role_delete($role) {

}


function hook_og_users_roles_grant($gid, $uid, $rid) {

}

function hook_og_users_roles_revoke($gid, $uid, $rid) {

}

/**
 * Allow modules to alter the groups that appear in the group audience field.
 *
 * @param $options
 *   Array passed by reference with the keys:
 *   - "content groups": Array with the group IDs that will appear to the user.
 *   - "other groups": Array with the group IDs that do not belong to the user,
 *     but if the user is an administrator they will see those groups as-well.
 * @param $opt_group
 *   TRUE if user should see also the "other groups" in the group audience
 *   field.
 * @param $account
 *   The user object.
 */
function hook_og_audience_options_alter(&$options, $opt_group, $account) {
  if (!$account->uid && $gids = og_register_get_groups()) {
    $options['content groups'] = array_merge($options['content groups'], $gids);
  }
}

/**
 * TODO
 */
function hook_og_fields_info() {

}

/**
 * TODO
 */
function hook_og_fields_info_alter(&$fields_info) {

}



/**
 * @} End of "addtgrouproup hooks".
 */