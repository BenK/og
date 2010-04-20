<?php
// $Id: group.admin.inc,v 1.15.4.2 2009/06/22 15:59:27 weitzman Exp $

/**
 * @file
 * Admin settings for the Organic groups module.
 */

/**
 * Provide an overview of the administrator menu items.
 *
 * @param $entity_type
 *   The entity type.
 * @param $etid
 *   The entity ID.
 */
function group_ui_group_admin_overview($entity_type, $etid) {
  group_set_breadcrumb($entity_type, $etid);

  $group = group_get_group($entity_type, $etid);
  $items = group_ui_get_group_admin($group->gid);

  foreach ($items as &$item) {
    // Re-format the URL.
    $item['href'] = "group/$entity_type/$etid/" . $item['href'];
    // Imitate the menu_get_item() content, so we can pass it to
    // theme_admin_block_content(), so we add the localized_options key.
    $item['localized_options'] = array();
  }

  $form['admin_block_content'] = array('#markup' => theme('admin_block_content', array('content' => $items)));

  return $form;
}

/**
 * Add users to group form.
 */
function group_ui_add_users($form, &$form_state, $entity_type, $etid) {
  group_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")));
  if ($group = group_get_group($entity_type, $etid)) {
    $label = check_plain(group_entity_get_label($group->entity_type, $group->etid));

    $form['group_user'] = array(
      '#type' => 'fieldset',
      '#title' => t('Add a group member to %group', array('%group' => $label)),
    );
    $form['group_user']['group_add_user'] = array(
      '#type' => 'textfield',
      '#title' => t('User name'),
      '#autocomplete_path' => 'user/autocomplete',
    );
    $form['group_user']['group_add_user_state'] = array(
      '#type' => 'select',
      '#title' => t('Membership state'),
      '#options' => group_group_content_states(),
      '#description' => t('Select the state of the membership.'),
    );
    $form['actions'] = array('#type' => 'container', '#attributes' => array('class' => array('form-actions')));
    $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Add users'));
    $form['group'] = array('#type' => 'value', '#value' => $group);
    return $form;
  }
  // Not a valid group node.
  drupal_not_found();
}

/**
 * Validate handler; Add users to group.
 */
function group_ui_add_users_validate($form, &$form_state) {
  $account = user_load_by_name($form_state['values']['group_add_user']);
  if (!$account) {
    form_set_error('group_add_user', t('You have entered an invalid user name.'));
  }
}

/**
 * Submit handler; Add users to group.
 */
function group_ui_add_users_submit($form, &$form_state) {
  $account = user_load_by_name($form_state['values']['group_add_user']);

  group_group($form_state['values']['group']->gid, 'user', $account, $form_state['values']['group_add_user_state']);
  drupal_set_message(t('%user has been added to group.', array('%user' => $account->name)));
}

/**
 * Form builder; GROUP user administration page.
 *
 * @ingroup forms
 * @see group_user_admin_account_validate()
 * @see group_user_admin_account_submit()
 */
function group_ui_user_admin_account($form, $form_state, $entity_type, $etid) {
  group_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")));
  $group = group_get_group($entity_type, $etid);
  $header = array(
    'username' => array('data' => t('Username'), 'field' => 'u.name'),
    'status' => array('data' => t('Status'), 'field' => 'group.group_audience_state'),
    'roles' => array('data' => t('Roles')),
    'member_for' => array('data' => t('Member for'), 'field' => 'group.group_audience_created', 'sort' => 'desc'),
  );

  $form['group'] = array('#type' => 'value', '#value' => $group);
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );
  $options = array();
  foreach (module_implements('group_user_operations') as $module) {
    $result = call_user_func($module . '_group_user_operations', array(), array('group' => $group));
    foreach ($result as $operation => $array) {
      $options[$operation] = $array['label'];
    }
  }

  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => 'unblock',
  );

  $options = array();
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $status = group_group_content_states();
  $roles = group_user_roles($group->gid);
  // Get the group real entity, so we can check the user ID of the entity.
  $entity = group_load_entity_from_group($group->gid);

  $query = db_select('field_data_group_audience', 'group');
  $query
    ->condition('group.group_audience_gid', $group->gid)
    ->condition('group.bundle', 'user');

  $count_query = clone $query;
  $count_query->addExpression('COUNT(group.bundle)');

  $query = $query->extend('PagerDefault')->extend('TableSort');
  $query->join('users', 'u', 'u.uid =  group.entity_id');
  $query
    ->fields('group', array('entity_id', 'group_audience_state', 'group_audience_created'))
    ->fields('u', array('name'))
    ->limit(10)
    ->orderByHeader($header)
    ->setCountQuery($count_query);
  $result = $query->execute()->fetchAll();

  $uids = array();
  foreach ($result as $row) {
    $uids[] = $row->entity_id;
  }

  if ($uids) {
    $states = group_group_content_states();
    // We load all the accounts as it's will happen in group_get_user_roles() and
    // we prefer loading all accounts at once.
    $accounts = user_load_multiple($uids);

    foreach ($result as $row) {
      $account = $accounts[$row->entity_id];
      // Don't allow editing of the group manager.
      if (empty($entity->uid) || $account->uid != $entity->uid) {
        $users_roles = array();
        foreach (group_get_user_roles($group->gid, $account->uid) as $rid) {
          if (!in_array($roles[$rid], array(GROUP_ANONYMOUS_ROLE, GROUP_AUTHENTICATED_ROLE))) {
            // Show the user's roles, except of the authenticated role, that all
            // group members have, or anonymous that blocked members have.
            $users_roles[] = $roles[$rid];
          }
        }
        asort($users_roles);

        $options[$account->uid] = array(
          'username' => theme('username', array('account' => $account)),
          'status' =>  $states[$row->group_audience_state],
          'roles' => theme('item_list', array('items' => $users_roles)),
          'member_for' => format_interval(REQUEST_TIME - $row->group_audience_created),
        );

      }
    }
    $form['accounts'] = array(
      '#type' => 'tableselect',
      '#header' => $header,
      '#options' => $options,
      '#empty' => !empty($entity->uid) ? t('No people available apart of the group manager.') : t('No people available.'),
    );
  }
  $form['pager'] = array('#markup' => theme('pager', array('tags' => NULL)));

  return $form;
}


function group_ui_user_admin_account_validate($form, &$form_state) {
  $form_state['values']['accounts'] = array_filter($form_state['values']['accounts']);
  if (count($form_state['values']['accounts']) == 0) {
    form_set_error('', t('No users selected.'));
  }
}

/**
 * Submit the user administration update form.
 */
function group_ui_user_admin_account_submit($form, &$form_state) {
  $group = $form_state['values']['group'];

  $operations = module_invoke_all('group_user_operations', $form, $form_state + array('group' => $group));
  $operation = $operations[$form_state['values']['operation']];
  // Filter out unchecked accounts.
  $accounts = array_filter($form_state['values']['accounts']);

  if ($function = $operation['callback']) {
    // Add in callback arguments if present.
    if (isset($operation['callback arguments'])) {
      $args = array_merge(array($accounts), $operation['callback arguments']);
    }
    else {
      $args = array($accounts);
    }
    call_user_func_array($function, array('gid' => $group->gid) + $args);

    drupal_set_message(t('The update has been performed.'));
  }
}

/**
 * Menu callback: administer roles.
 *
 * @ingroup forms
 * @see group_user_admin_role_validate()
 * @see group_user_admin_role_submit()
 * @see theme_group_user_admin_new_role()
 */
function group_ui_user_admin_role($form, $form_state, $entity_type, $etid) {
  if ($group = group_get_group($entity_type, $etid)) {
    $form['group'] = array('#type' => 'value', '#value' => $group);

    $item = menu_get_item();
    // Get the role ID from the URL.
    $rid = !empty($item['map'][7]) ? $item['map'][7] : 0;

    if ($rid) {
      group_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")), l(t('Roles'), "group/$entity_type/$etid/admin/people/roles"));

      $group_roles = group_user_roles($group->gid);
      if (!in_array($rid, array_keys($group_roles))) {
        drupal_goto("group/$entity_type, $etid/admin/people/roles");
      }
      // Display the edit role form.
      $role = group_user_role_load($rid);
      $form['name'] = array(
        '#type' => 'textfield',
        '#title' => t('Role name'),
        '#default_value' => $role->name,
        '#size' => 30,
        '#required' => TRUE,
        '#maxlength' => 64,
        '#description' => t('The name for this role. Example: "moderator", "editorial board", "site architect".'),
      );
      $form['rid'] = array(
        '#type' => 'value',
        '#value' => $rid,
      );
      $form['actions'] = array('#type' => 'container', '#attributes' => array('class' => array('form-actions')));
      $form['actions']['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save role'),
      );
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete role'),
      );
    }
    else {
      group_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")));
      if (!group_is_group_default_access($group->gid)) {
        $form['name'] = array(
          '#type' => 'textfield',
          '#size' => 32,
          '#maxlength' => 64,
        );
        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => t('Add role'),
        );
        $form['#submit'][] = 'group_ui_user_admin_role_submit';
        $form['#validate'][] = 'group_ui_user_admin_role_validate';
      }
    }
    return $form;
  }
  else {
    // Not a group node.
    drupal_not_found();
    exit;
  }
}

function group_ui_user_admin_role_validate($form, &$form_state) {
  if (!empty($form_state['values']['name'])) {
    $roles = group_user_roles($form_state['values']['group']->gid);
    $unique_name = TRUE;
    if (!empty($form_state['values']['rid'])) {
      $role = group_user_role_load($form_state['values']['rid']);
      $unique_name = $role->name != $form_state['values']['name'];
    }
    if ($unique_name && in_array($form_state['values']['name'], $roles)) {
      form_set_error('name', t('The role name %name already exists. Please choose another role name.', array('%name' => $form_state['values']['name'])));
    }
  }
}

function group_ui_user_admin_role_submit($form, &$form_state) {
  $group = $form_state['values']['group'];
  $role = new stdClass();
  $role->gid = $group->gid;
  $role->name = $form_state['values']['name'];
  if (!empty($form_state['values']['rid'])) {
    $role->rid= $form_state['values']['rid'];
  }
  if ($form_state['values']['op'] == t('Save role')) {
    group_user_role_save($role);
    drupal_set_message(t('The role has been renamed.'));
  }
  elseif ($form_state['values']['op'] == t('Delete role')) {
    group_user_role_delete($form_state['values']['rid']);
    drupal_set_message(t('The role has been deleted.'));
  }
  elseif ($form_state['values']['op'] == t('Add role')) {
    group_user_role_save($role);
    drupal_set_message(t('The role has been added.'));
  }
  $form_state['redirect'] = 'group/' . $group->entity_type . '/' . $group->etid . '/admin/people/roles';
  return;
}

/**
 * Theme the new role form.
 *
 * @ingroup themeable
 */
function theme_group_ui_user_admin_new_role($variables) {
  $form = $variables['form'];
  $header = array(t('Name'), array('data' => t('Operations'), 'colspan' => 2));
  // The group entity.
  $group = $form['group']['#value'];

  $default_roles = group_is_group_default_access($group->gid);

  foreach (group_user_roles($group->gid) as $rid => $name) {
    $text = !$default_roles ? t('edit permissions') : t('view permissions');
    $permissions = l($text, 'group/' . $group->entity_type .'/' . $group->etid . '/admin/people/permissions/' . $rid);

    if (!$default_roles && !in_array($name, array(GROUP_ANONYMOUS_ROLE, GROUP_AUTHENTICATED_ROLE))) {
      $rows[] = array($name, l(t('edit role'), 'group/' . $group->entity_type .'/' . $group->etid . '/admin/people/roles/edit/' . $rid), $permissions);
    }
    else {
      $rows[] = array($name, t('locked'), $permissions);
    }
  }

  $rows[] = array(array('data' => drupal_render($form['name']) . drupal_render($form['submit']), 'colspan' => 3, 'class' => 'edit-name'));

  $output = drupal_render_children($form);
  $output .= theme('table', array('header' => $header, 'rows' => $rows));

  return $output;
}


/**
 * Menu callback: administer permissions.
 *
 * @ingroup forms
 * @see group_ui_user_admin_permissions_submit()
 * @see theme_group_ui_user_admin_permissions()
 */
function group_ui_user_admin_permissions($form, $form_state, $entity_type = NULL, $etid = NULL, $rid = NULL) {
  // If no node entity is provided then the node ID is 0, which means this
  // is the default permissions settings.
  $group = !empty($etid) ? group_get_group($entity_type, $etid) : array();
  $form['group'] = array('#type' => 'value', '#value' => $group);

  if (!empty($group)) {
    $gid =  $group->gid;
    group_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")));
  }
  else {
    $gid = 0;
  }

  // Retrieve role names for columns.
  $role_names = group_user_roles($gid);
  if (!empty($rid)) {
    $role_names = array($rid => $role_names[$rid]);
  }
  // Fetch permissions for all roles or the one selected role.
  $role_permissions = group_user_role_permissions($role_names);

  // Store $role_names for use when saving the data.
  $form['role_names'] = array(
    '#type' => 'value',
    '#value' => $role_names,
  );
  // Render role/permission overview:
  $options = array();
  foreach (group_get_permissions() as $module => $permissions) {
    $form['permission'][] = array('#markup' => $module != 'administer' ? $module : t('administer group admins privileges'), '#id' => $module);
    foreach ($permissions as $perm => $perm_item) {
      $access = !empty($group) ? group_user_access($gid, 'show ' . $perm) : TRUE;
      if ($access) {
        $options[$perm] = '';
        $form['permission'][$perm] = array(
          '#type' => 'item',
          '#markup' => $perm_item['title'],
          '#description' => $perm_item['description'],
        );
        foreach ($role_names as $rid => $name) {
          // Builds arrays for checked boxes for each role
          if (isset($role_permissions[$rid][$perm])) {
            $status[$rid][] = $perm;
          }
        }
      }
    }
  }

  // Have to build checkboxes here after checkbox arrays are built
  foreach ($role_names as $rid => $name) {
    $form['checkboxes'][$rid] = array('#type' => 'checkboxes', '#options' => $options, '#default_value' => isset($status[$rid]) ? $status[$rid] : array());
    $form['role_names'][$rid] = array('#markup' => $name, '#tree' => TRUE);
  }

  if (empty($group)) {
    $form['#theme'] = array('group_ui_user_admin_permissions');
  }
  if ($gid == 0 || !group_is_group_default_access($group->gid)) {
    $form['actions'] = array('#type' => 'container', '#attributes' => array('class' => array('form-actions')));
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => $gid == 0 ? t('Save global permissions') : t('Save permissions'),
      '#submit' => array('group_ui_user_admin_permissions_submit'),
    );
  }

  $form['#after_build'][] = 'group_ui_user_admin_permissions_after_build';

  return $form;
}

/**
 * Helper function to disable the permissions checkboxes.
 *
 * Every permission can declare to which roles it applies, as some don't make
 * sense if applied to anonymous or authenticated user (e.g. subscribe to group
 * should appear only to anonymous members).
 * A second case for disabling is when using deafult roles and permissions, and
 * the user is browsing a group's permissions page.
 */
function group_ui_user_admin_permissions_after_build($element) {
  $gid = !empty($element['group']['#value']->gid) ? $element['group']['#value']->gid : 0;

  // Indicate if all checkbes should be disabled.
  $disable = $gid && group_is_group_default_access($gid);

  // Authenticated roles.
  $auth_rids = array_flip(group_user_roles($gid));

  // Remove the anonymous member from the authenticated roles.
  $anon_rid = array_shift($auth_rids);

  foreach (group_get_permissions() as $module_perms) {
    foreach ($module_perms as $key => $perm) {
      $perm['roles'] = drupal_map_assoc($perm['roles']);

      if ($disable || (empty($perm['roles']) || empty($perm['roles'][GROUP_ANONYMOUS_ROLE]))) {
        $element['checkboxes'][$anon_rid][$key]['#attributes']['disabled'] = TRUE;
      }
      if ($disable || (empty($perm['roles']) || empty($perm['roles'][GROUP_AUTHENTICATED_ROLE]))) {
        foreach ($auth_rids as $auth_rid) {
          $element['checkboxes'][$auth_rid][$key]['#attributes']['disabled'] = TRUE;
        }
      }
    }
  }

  return $element;
}
/**
 * Save permissions selected on the administer permissions page.
 *
 * @see group_user_admin_permissions()
 */
function group_ui_user_admin_permissions_submit($form, &$form_state) {
  foreach ($form_state['values']['role_names'] as $rid => $name) {
    group_user_role_change_permissions($rid, $form_state['values'][$rid]);
  }

  drupal_set_message(t('The changes have been saved.'));
}


/**
 * Theme the administer permissions page.
 *
 * @ingroup themeable
 */
function theme_group_ui_user_admin_permissions($variables) {
  $form = $variables['form'];
  $gid = !empty($form['group']['#value']->gid) ? $form['group']['#value']->gid : 0;
  $roles = group_user_roles($gid);

  foreach (element_children($form['permission']) as $key) {
    $row = array();
    // Module name.
    if (is_numeric($key)) {
      $row[] = array('data' => drupal_render($form['permission'][$key]), 'class' => array('module'), 'id' => 'module-' . $form['permission'][$key]['#id'], 'colspan' => count($form['role_names']['#value']) + 1);
    }
    else {
      // Permission row.
      $row[] = array(
        'data' => drupal_render($form['permission'][$key]),
        'class' => array('permission'),
      );
      foreach (element_children($form['checkboxes']) as $rid) {
        $row[] = array('data' => drupal_render($form['checkboxes'][$rid][$key]), 'class' => array('checkbox'), 'title' => $roles[$rid] . ' : ' . t($key));
      }
    }
    $rows[] = $row;
  }
  $header[] = (t('Permission'));
  foreach (element_children($form['role_names']) as $rid) {
    $header[] = array('data' => drupal_render($form['role_names'][$rid]), 'class' => array('checkbox'));
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'permissions')));
  $output .= drupal_render_children($form);
  return $output;
}


/**
 * Groups permissions on default settings form.
 *
 * Allow site admin to set which permissions are available on a per group basis.
 */
function group_ui_field_settings() {
  $form = array();
  $options = array();

  $form['group_group'] = array(
    '#type' => 'fieldset',
    '#title' => t('Group type'),
  );
  $form['group_group']['group_group'] = _group_admin_fields_overview('group');

  $form['group_group_content'] = array(
    '#type' => 'fieldset',
    '#title' => t('Group post type'),
  );
  $form['group_group_content']['group_group_content'] = _group_admin_fields_overview('group content');

  $form['actions'] = array('#type' => 'container', '#attributes' => array('class' => array('form-actions')));
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add group and group content fields to bundles'),
    '#submit' => array('group_ui_field_settings_submit'),
  );

  return $form;
}

/**
 * Helper function to create tables for group and group content fields.
 *
 * @param $group_type
 * @return unknown_type
 */
function _group_admin_fields_overview($group_type = 'group') {
  $form = array();
  $options = $status = array();

  $group_fields = group_fields_info();

  foreach (entity_get_info() as $entity => $entity_value) {
    if (!empty($entity_value['fieldable'])) {
      $form['bundle'][] = array('#markup' => t('@entity entity', array('@entity' => $entity_value['label'])), '#id' => $entity);
      foreach($entity_value['bundles'] as $bundle => $bundle_value) {
        $options[$bundle] = '';
        $form['bundle'][$bundle] = array(
          '#type' => 'item',
          '#markup' => '-- ' . check_plain($bundle_value['label']),
        );
      }
    }
  }


  // Add the fields that exist in the bundle.
  foreach (field_info_fields() as $field => $value) {
    if (!empty($value['bundles'])) {
      foreach ($value['bundles'] as $entity) {
        foreach ($entity as $bundle) {
          $status[$field][$bundle] = $bundle;
        }
      }
    }
  }

  // Have to build checkboxes here after checkbox arrays are built
  foreach ($group_fields as $field => $field_value) {
    if (in_array($group_type, $field_value['type'])) {
      $form['checkboxes'][$field] = array('#type' => 'checkboxes', '#options' => $options, '#default_value' => !empty($status[$field]) ? $status[$field] : array());
      $form['modules'][$field] = array('#markup' =>  $field_value['instance']['label'] . ' (' . $field_value['module'] . ')', '#tree' => TRUE);
    }
  }

  $form['#theme'] = array('group_ui_user_admin_bundles');

  return $form;
}

function group_ui_field_settings_submit($form, &$form_state) {
  $messages = array();
  $group_fields = group_fields_info();
  $bundle_info = array();
  foreach (field_info_bundles() as $entity => $bundles) {
    foreach ($bundles as $key => $value) {
        // Get a two dimension array with the bundle as key and the entity
        // as the value.
      $bundle_info[$key] = $entity;
    }
  }

  foreach ($group_fields as $field_name => $field_value) {
    if (!empty($form_state['values'][$field_name]) && $bundles = array_filter($form_state['values'][$field_name])) {
      foreach ($bundles as $key => $bundle) {
        // Add the field if it doesn't already exist.
        if (!field_info_instance($bundle_info[$bundle], $field_name, $bundle)) {
          call_user_func('group_create_field', $field_name, $bundle_info[$bundle], $bundle);
          $messages[] = t('Added field %field to %bundle.', array('%field' => $group_fields[$field_name]['instance']['label'], '%bundle' => $bundle));
        }
      }
    }
  }

  if (!empty($messages)) {
    foreach ($messages as $message) {
      drupal_set_message($message);
    }
  }
}

/**
 * Theme the administer bundles page.
 *
 * @ingroup themeable
 */
function theme_group_ui_user_admin_bundles($variables) {
  $form = $variables['form'];

  $modules = array_keys(element_children($form['modules']));

  foreach (element_children($form['bundle']) as $key) {
    $row = array();
    // Module name.
    if (is_numeric($key)) {
      $row[] = array('data' => drupal_render($form['bundle'][$key]), 'class' => array('module'), 'id' => 'module-' . $form['bundle'][$key]['#id'], 'colspan' => count($modules) + 1);
    }
    else {
      // bundle row.
      $row[] = array(
        'data' => drupal_render($form['bundle'][$key]),
        'class' => array('bundle'),
      );
      foreach (element_children($form['checkboxes']) as $module) {
        $row[] = array('data' => drupal_render($form['checkboxes'][$module][$key]), 'class' => array('checkbox'), 'title' => t('@bundle bundle', array('@bundle' => $key)));
      }
    }
    $rows[] = $row;
  }
  $header[] = (t('bundle'));
  foreach (element_children($form['modules']) as $module) {
    $header[] = array('data' => drupal_render($form['modules'][$module]), 'class' => array('checkbox'));
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'bundles')));
  $output .= drupal_render_children($form);
  return $output;
}